#include "../include/vermell/util/sysprocess.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>

#include <grp.h>
#include <pwd.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#include "../include/vermell/util/nterminal.h"

const char* const neosys::process::log_path = "log_cv.log";
// The child's environment is replaced wholesale: give it a sane, fixed PATH
// (the previous "PATH=$PATH:..." string was passed literally, since execve
// never expands variables).
const std::string neosys::process::path = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";


std::mt19937& neosys::process::get_rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

unsigned long neosys::process::random()  {
    static std::uniform_int_distribution<std::mt19937::result_type> dist;
    return dist(get_rng());
}

namespace {

    // Applies one rlimit in the child; failures abort the exec so a
    // half-sandboxed program never runs.
    bool apply_limit(const int resource, const rlim_t value) {
        if (value == 0)
            return true;
        const rlimit lim{value, value};
        return setrlimit(resource, &lim) == 0;
    }

    void child_setup(const neosys::RunOptions &opts) {
        if (opts.new_session)
            setsid(); // own process group: the parent can SIGKILL all of it

        umask(077);
        const rlimit no_core{0, 0};
        setrlimit(RLIMIT_CORE, &no_core);

        if (!apply_limit(RLIMIT_CPU,    opts.cpu_seconds)     ||
            !apply_limit(RLIMIT_AS,     opts.memory_bytes)    ||
            !apply_limit(RLIMIT_FSIZE,  opts.file_size_bytes) ||
            !apply_limit(RLIMIT_NOFILE, opts.no_files)        ||
            !apply_limit(RLIMIT_NPROC,  opts.max_processes))
            _Exit(127);

        if (opts.work_dir != nullptr && chdir(opts.work_dir) != 0)
            _Exit(127);

        // Do not leak server fds (listening socket, epoll, files) into the
        // child: everything above stderr goes away. sysconf() may return -1
        // ("indeterminate"): fall back to a sane bound instead of skipping.
        long open_max = sysconf(_SC_OPEN_MAX);
        if (open_max < 0)
            open_max = 4096;
        if (open_max > 4096)
            open_max = 4096;
        for (long fd = 3; fd < open_max; ++fd)
            close(static_cast<int>(fd));
    }

    // Resolves the unprivileged drop target. Called only in the parent,
    // before fork(): getpwnam()/getgrnam() are not async-signal-safe.
    uid_t nobody_uid() {
        if (const passwd* pw = ::getpwnam("nobody"); pw != nullptr)
            return pw->pw_uid;
        return static_cast<uid_t>(65534);
    }

    gid_t nobody_gid() {
        if (const group* gr = ::getgrnam("nogroup"); gr != nullptr)
            return gr->gr_gid;
        return static_cast<gid_t>(65534);
    }

    // Drops from root to the "nobody" user. Only async-signal-safe calls.
    void drop_privileges(const uid_t uid, const gid_t gid) {
        if (::getuid() != 0)
            return; // already unprivileged: nothing to drop
        if (::setgroups(0, nullptr) != 0)
            _Exit(127);
        if (::setgid(gid) != 0)
            _Exit(127);
        if (::setuid(uid) != 0)
            _Exit(127);
        if (::setuid(0) == 0)
            _Exit(127); // the drop must be irreversible
    }

    // ---- execution sandbox (readFileX run step) ---------------------------
    //
    // Two independent layers, both best-effort:
    //   1. seccomp: denies networking and privileged/escape syscalls for the
    //      whole process tree (filters are inherited across fork/exec). This
    //      is what actually stops exfiltration, scanning and sandbox escape,
    //      and it works unprivileged.
    //   2. namespaces: CLONE_NEWNET when root; for non-root, the
    //      user-namespace dance (CLONE_NEWUSER + uid/gid mapping) first so
    //      the network namespace can be created without privileges. Failure
    //      here only degrades isolation — the seccomp layer still blocks
    //      networking.

    // Decimal formatter into a caller buffer (no heap, no locale: the child
    // must not allocate between fork() and execve()).
    void write_uint(char* buf, const size_t cap, const unsigned long v) {
        (void)cap; // callers always pass buffers large enough for a uid/gid
        char tmp[24];
        size_t n = 0;
        unsigned long x = v;
        do { tmp[n++] = static_cast<char>('0' + x % 10); x /= 10; } while (x != 0 && n < sizeof(tmp));
        size_t i = 0;
        while (n > 0) buf[i++] = tmp[--n];
        buf[i] = '\0';
    }

    bool write_small_file(const char* path, const char* data, const size_t len) {
        const int fd = ::open(path, O_WRONLY);
        if (fd < 0)
            return false;
        const ssize_t w = ::write(fd, data, len);
        ::close(fd);
        return w == static_cast<ssize_t>(len);
    }

    // Writes "/proc/self/uid_map" / "gid_map" with the single mapping
    // "0 <real-id> 1": inside the new user namespace the child is uid 0, but
    // the kernel maps that to the server user for file access, so templates
    // keep working with the server user's permissions.
    bool map_namespace_id(const char* proc_file, const unsigned long real_id) {
        char line[40];
        size_t n = 0;
        line[n++] = '0';
        line[n++] = ' ';
        char digits[24];
        write_uint(digits, sizeof(digits), real_id);
        for (size_t i = 0; digits[i] != '\0' && n + 1 < sizeof(line); ++i)
            line[n++] = digits[i];
        line[n++] = ' ';
        line[n++] = '1';
        line[n++] = '\n';
        return write_small_file(proc_file, line, n);
    }

    void try_namespace_isolation(const bool is_root) {
        if (is_root) {
            // root can create a network namespace directly.
            (void)::syscall(SYS_unshare, CLONE_NEWNET);
            return;
        }
        // Non-root: user namespace first (grants CAP_SYS_ADMIN inside it),
        // map the real uid/gid so file access still works, then netns.
        if (::syscall(SYS_unshare, CLONE_NEWUSER) != 0)
            return;
        // Order matters for unprivileged callers: setgroups must be "deny"
        // before gid_map can be written.
        (void)write_small_file("/proc/self/setgroups", "deny", 4);
        if (!map_namespace_id("/proc/self/uid_map", static_cast<unsigned long>(::getuid())))
            return;
        if (!map_namespace_id("/proc/self/gid_map", static_cast<unsigned long>(::getgid())))
            return;
        (void)::syscall(SYS_unshare, CLONE_NEWNET);
    }

    // ---- seccomp filter ----

#if defined(__x86_64__)
    constexpr unsigned kAuditArch = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
    constexpr unsigned kAuditArch = AUDIT_ARCH_AARCH64;
#else
    constexpr unsigned kAuditArch = 0; // unknown: the filter is never installed
#endif

    // One deny entry: JEQ <nr> -> return EPERM, else fall through.
    #define VERMELL_DENY_SYSCALL(nr) \
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<std::uint32_t>(nr), 0, 1), \
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA))

    // Denied syscall families:
    //   - networking (socket/bind/connect/...): no exfiltration, no scanning,
    //     no C2 — the core of the sandbox
    //   - session/group manipulation (setsid/setpgid): descendants stay in
    //     the process group the timeout SIGKILLs
    //   - privilege/namespace escape and kernel control: ptrace, mount,
    //     unshare, keyrings, bpf, perf, module loading, ...
    // Everything else (file I/O, process spawn via system(), ...) is allowed.
    static const struct sock_filter kSandboxFilter[] = {
        // Load the architecture: if it does not match this build, allow (the
        // __NR_* numbers below are arch-specific and would be wrong there).
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, kAuditArch, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
#ifdef __NR_socket
        VERMELL_DENY_SYSCALL(__NR_socket),
#endif
#ifdef __NR_socketpair
        VERMELL_DENY_SYSCALL(__NR_socketpair),
#endif
#ifdef __NR_connect
        VERMELL_DENY_SYSCALL(__NR_connect),
#endif
#ifdef __NR_bind
        VERMELL_DENY_SYSCALL(__NR_bind),
#endif
#ifdef __NR_listen
        VERMELL_DENY_SYSCALL(__NR_listen),
#endif
#ifdef __NR_accept
        VERMELL_DENY_SYSCALL(__NR_accept),
#endif
#ifdef __NR_accept4
        VERMELL_DENY_SYSCALL(__NR_accept4),
#endif
#ifdef __NR_sendto
        VERMELL_DENY_SYSCALL(__NR_sendto),
#endif
#ifdef __NR_sendmsg
        VERMELL_DENY_SYSCALL(__NR_sendmsg),
#endif
#ifdef __NR_sendmmsg
        VERMELL_DENY_SYSCALL(__NR_sendmmsg),
#endif
#ifdef __NR_recvfrom
        VERMELL_DENY_SYSCALL(__NR_recvfrom),
#endif
#ifdef __NR_recvmsg
        VERMELL_DENY_SYSCALL(__NR_recvmsg),
#endif
#ifdef __NR_recvmmsg
        VERMELL_DENY_SYSCALL(__NR_recvmmsg),
#endif
#ifdef __NR_shutdown
        VERMELL_DENY_SYSCALL(__NR_shutdown),
#endif
#ifdef __NR_getsockopt
        VERMELL_DENY_SYSCALL(__NR_getsockopt),
#endif
#ifdef __NR_setsockopt
        VERMELL_DENY_SYSCALL(__NR_setsockopt),
#endif
#ifdef __NR_getsockname
        VERMELL_DENY_SYSCALL(__NR_getsockname),
#endif
#ifdef __NR_getpeername
        VERMELL_DENY_SYSCALL(__NR_getpeername),
#endif
#ifdef __NR_sendfile
        VERMELL_DENY_SYSCALL(__NR_sendfile),
#endif
#ifdef __NR_setsid
        VERMELL_DENY_SYSCALL(__NR_setsid),
#endif
#ifdef __NR_setpgid
        VERMELL_DENY_SYSCALL(__NR_setpgid),
#endif
#ifdef __NR_ptrace
        VERMELL_DENY_SYSCALL(__NR_ptrace),
#endif
#ifdef __NR_process_vm_readv
        VERMELL_DENY_SYSCALL(__NR_process_vm_readv),
#endif
#ifdef __NR_process_vm_writev
        VERMELL_DENY_SYSCALL(__NR_process_vm_writev),
#endif
#ifdef __NR_keyctl
        VERMELL_DENY_SYSCALL(__NR_keyctl),
#endif
#ifdef __NR_add_key
        VERMELL_DENY_SYSCALL(__NR_add_key),
#endif
#ifdef __NR_request_key
        VERMELL_DENY_SYSCALL(__NR_request_key),
#endif
#ifdef __NR_bpf
        VERMELL_DENY_SYSCALL(__NR_bpf),
#endif
#ifdef __NR_perf_event_open
        VERMELL_DENY_SYSCALL(__NR_perf_event_open),
#endif
#ifdef __NR_mount
        VERMELL_DENY_SYSCALL(__NR_mount),
#endif
#ifdef __NR_umount2
        VERMELL_DENY_SYSCALL(__NR_umount2),
#endif
#ifdef __NR_pivot_root
        VERMELL_DENY_SYSCALL(__NR_pivot_root),
#endif
#ifdef __NR_chroot
        VERMELL_DENY_SYSCALL(__NR_chroot),
#endif
#ifdef __NR_init_module
        VERMELL_DENY_SYSCALL(__NR_init_module),
#endif
#ifdef __NR_finit_module
        VERMELL_DENY_SYSCALL(__NR_finit_module),
#endif
#ifdef __NR_delete_module
        VERMELL_DENY_SYSCALL(__NR_delete_module),
#endif
#ifdef __NR_kexec_load
        VERMELL_DENY_SYSCALL(__NR_kexec_load),
#endif
#ifdef __NR_reboot
        VERMELL_DENY_SYSCALL(__NR_reboot),
#endif
#ifdef __NR_swapon
        VERMELL_DENY_SYSCALL(__NR_swapon),
#endif
#ifdef __NR_swapoff
        VERMELL_DENY_SYSCALL(__NR_swapoff),
#endif
#ifdef __NR_setns
        VERMELL_DENY_SYSCALL(__NR_setns),
#endif
#ifdef __NR_unshare
        VERMELL_DENY_SYSCALL(__NR_unshare),
#endif
#ifdef __NR_open_by_handle_at
        VERMELL_DENY_SYSCALL(__NR_open_by_handle_at),
#endif
#ifdef __NR_name_to_handle_at
        VERMELL_DENY_SYSCALL(__NR_name_to_handle_at),
#endif
#ifdef __NR_fanotify_init
        VERMELL_DENY_SYSCALL(__NR_fanotify_init),
#endif
#ifdef __NR_fanotify_mark
        VERMELL_DENY_SYSCALL(__NR_fanotify_mark),
#endif
#ifdef __NR_sethostname
        VERMELL_DENY_SYSCALL(__NR_sethostname),
#endif
#ifdef __NR_setdomainname
        VERMELL_DENY_SYSCALL(__NR_setdomainname),
#endif
#ifdef __NR_ioperm
        VERMELL_DENY_SYSCALL(__NR_ioperm),
#endif
#ifdef __NR_iopl
        VERMELL_DENY_SYSCALL(__NR_iopl),
#endif
#ifdef __NR_acct
        VERMELL_DENY_SYSCALL(__NR_acct),
#endif
#ifdef __NR_quotactl
        VERMELL_DENY_SYSCALL(__NR_quotactl),
#endif
#ifdef __NR_personality
        VERMELL_DENY_SYSCALL(__NR_personality),
#endif
#ifdef __NR_uselib
        VERMELL_DENY_SYSCALL(__NR_uselib),
#endif
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    #undef VERMELL_DENY_SYSCALL

    // Installs the filter. Returns false when seccomp is unavailable or the
    // architecture is unknown (the caller keeps running, just less isolated).
    bool install_seccomp_filter() {
#if defined(__x86_64__) || defined(__aarch64__)
        // No new privileges: execve() can never regain capabilities, so the
        // filter cannot be bypassed by the executed binary.
        if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
            return false;
        const struct sock_fprog prog = {
            static_cast<unsigned short>(sizeof(kSandboxFilter) / sizeof(kSandboxFilter[0])),
            const_cast<struct sock_filter*>(kSandboxFilter),
        };
        return ::syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) == 0;
#else
        return false; // unknown arch: no filter (documented limitation)
#endif
    }

} // namespace

int neosys::process::run_command(const std::vector<const char*> &args,
                                 const std::string& _path,
                                 const RunOptions& opts) {
    int status;

    if (args.empty() || !static_cast<bool>(args[0])) {
        return VER_NVALUE;
    }

    // Resolve the drop target before forking: getpwnam/getgrnam are not
    // async-signal-safe, so the child must never call them.
    const uid_t drop_uid = opts.drop_privileges ? nobody_uid() : static_cast<uid_t>(0);
    const gid_t drop_gid = opts.drop_privileges ? nobody_gid() : static_cast<gid_t>(0);

    if (const pid_t pid = fork(); pid == VER_NVALUE) {
        return VER_NVALUE;
    } else if (pid != 0) {
        // ---- parent ----
        if (opts.timeout.count() <= 0) {
            // Legacy: block until the child finishes.
            pid_t retval;
            while ((retval = waitpid(pid, &status, 0)) != VER_NVALUE) {
                if (retval == pid) break;
                if (errno == EINTR) continue;
            }
            if (retval == VER_NVALUE || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                return VER_NVALUE;
            }
            return 0;
        }

        // Bounded wait: poll the child and kill it once the budget burns out.
        const auto deadline = std::chrono::steady_clock::now() + opts.timeout;
        for (;;) {
            const pid_t retval = waitpid(pid, &status, WNOHANG);
            if (retval == pid) {
                return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : VER_NVALUE;
            }
            if (retval == VER_NVALUE) {
                if (errno == EINTR)
                    continue;
                return VER_NVALUE;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                if (opts.new_session)
                    kill(-pid, SIGKILL); // the whole group first
                kill(pid, SIGKILL);      // fallback if setsid() failed
                while (waitpid(pid, &status, 0) == VER_NVALUE && errno == EINTR)
                    ;
                return VER_NVALUE;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    } else {
        // ---- child ----
        child_setup(opts);

        // Open the output/log file BEFORE dropping privileges: it is created
        // (0600) by the server user, stays readable by the parent, and the
        // template only writes through the already-open descriptor.
        const int fd = open(_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd == VER_NVALUE) {
            _Exit(127);
        }

        if(dup2(fd, STDOUT_FILENO) == VER_NVALUE) {
            close(fd);
            _Exit(127);
        }
        if(dup2(fd, STDERR_FILENO) == VER_NVALUE) {
            close(fd);
            _Exit(127);
        }
        close(fd);

        // stdin from /dev/null: the executed program must not read (or block
        // on) the server's stdin.
        const int nullfd = open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO);
            close(nullfd);
        }

        // True when the child was launched by root: the only case in which
        // privileges can be dropped — or a network namespace created without
        // the user-namespace dance. Computed BEFORE any unshare().
        const bool is_root = (::getuid() == 0);

        if (opts.sandbox) {
            // Execution sandbox (readFileX run step): network/privilege
            // seccomp for the whole tree plus best-effort namespaces.
            try_namespace_isolation(is_root);
            if (opts.drop_privileges && is_root)
                drop_privileges(drop_uid, drop_gid);
            install_seccomp_filter();
        } else if (opts.drop_privileges) {
            drop_privileges(drop_uid, drop_gid);
        }

        // No heap allocations between fork() and execve(): malloc is not
        // async-signal-safe and another worker thread may hold the arena
        // lock at the moment of the fork (deadlock). A stack array is enough.
        constexpr size_t MAX_ARGS = 256;
        if (args.size() >= MAX_ARGS)
            _Exit(127);

        std::array<char*, MAX_ARGS> argv{};
        for (size_t i = 0; i < args.size(); ++i)
            argv[i] = const_cast<char*>(args[i]);
        argv[args.size()] = nullptr;

        const std::array<const char*, 2> export_path = {path.c_str(), nullptr};
        if (execve(argv[0], argv.data(), const_cast<char* const*>(export_path.data())) == VER_NVALUE) {
            _Exit(127);
        }
    }
    return 0;
}



std::string neosys::process::readFile(const std::string &path, char separator) {
    std::ifstream reader(path, std::ios::binary);
    if (!reader.is_open())
        return {};

    // Chunked read: works for regular files and for size-lying pseudo-files
    // (/proc), and does not trip the istreambuf_iterator null-dereference
    // false positive that libstdc++ emits at -O3.
    std::string body;
    std::array<char, 16384> chunk{};
    while (reader) {
        reader.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        body.append(chunk.data(), static_cast<size_t>(reader.gcount()));
    }

    if (separator != '\0')
        std::replace(body.begin(), body.end(), '\n', separator);

    return body;
}

int neosys::process::writeFile(const std::string &path, const std::string &content) {
    std::ofstream write_stream(path, std::ios::binary);
    if (!write_stream.is_open())
        return VER_NVALUE;

    write_stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    write_stream.close();
    return write_stream.good() ? VER_OK : VER_NVALUE;
}
