#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <string>

struct notify_html {
      static std::string noPath() noexcept {
        return "Vibe: error no se puedo encontrar la ruta! <br/> Vibe: error cant get the path! ";
      }
      static std::string noFIle(const std::string& name) noexcept {
        return "Vibe: error no se puedo encontrar el archivo " + name + " <br/> Vibe: error cant get the file  " + name;
      }

      static std::string noSafe() noexcept {
        return "Vibe: error de sintaxis no <strong>'];'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>'];'</strong>  ";
      }

      static std::string noSafeData() noexcept {
        return "Vibe: error de sintaxis no <strong>']]'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>']]'</strong>  ";
      }
};

struct notify {
  static std::string noPath(const string& path) noexcept {
    return  "Can't get path file. '" + path + "',  ";
  }
};

#endif // ! NOTIFY_HPP