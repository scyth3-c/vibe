# Commit Message Format

This repository uses a specific format for commit messages. Following this format helps maintain an organized and readable commit history. The format is as follows:

```
<type>(<scope>): <subject>
```

## Example:

```
feat(router): add support for new route parameters
```

## Message Structure

Each commit message should contain the following parts:

1. **type**: The type of change being made.
2. **scope**: The specific section of the project affected by the change (optional but recommended).
3. **subject**: A brief description of the change, in the present tense and without a period at the end.

### Commit Types

- **feat**: A new feature for the framework.
- **fix**: A bug fix.
- **docs**: Documentation changes (README, comments, etc.).
- **style**: Changes that do not affect the code's logic (whitespace, formatting, etc.).
- **refactor**: Code changes that neither fix bugs nor add features but improve the structure.
- **perf**: Changes that improve performance.
- **test**: Adding or correcting tests.
- **build**: Changes that affect the build system or external dependencies.
- **ci**: Changes to CI configuration files and scripts.
- **chore**: Minor updates or administrative tasks that do not affect the code’s logic.
- **revert**: Reverts a previous commit.

### Scope

The optional **scope** can include the name of the file, folder, or specific module affected by the change. Examples:

- `router`: Changes in routing.
- `middlewares`: Changes in middlewares.
- `sockets`: Modifications to socket handling.
- `security`: Updates in security modules.
- `examples`: Changes in the examples included in the project.
- `build`: Changes in the build system.

### Subject

The **subject** should be a brief description of what the commit does. Use the present tense, for example:

- `fix(router): resolve issue with route matching`
- `feat(middlewares): add new logging middleware`
- `refactor(parameters): simplify parameter processing logic`

### Commit Body (Optional)

The commit body is optional and can be used to explain the changes in more detail. It should be used when the change is not trivial and requires additional explanation.

---

Following this structure helps maintain a clear and consistent commit history, which is crucial for project management and collaboration.
