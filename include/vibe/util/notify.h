#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <string>

#include "secure_render.h"

// Names/paths reflected into a response body must be HTML-escaped and
// stripped of control characters: an unescaped name is an XSS sink when
// the route serves text/html.

struct notify_html {
      static std::string noPath() noexcept {
        return "Vibe: error no se puedo encontrar la ruta! <br/> Vibe: error cant get the path! ";
      }
      static std::string noFIle(const std::string& name) noexcept {
        return "Vibe: error no se puedo encontrar el archivo " + vibe::srender::escape_html(name)
             + " <br/> Vibe: error cant get the file  " + vibe::srender::escape_html(name);
      }

      static std::string badName(const std::string& name) noexcept {
        return "Vibe: nombre de modulo no permitido " + vibe::srender::escape_html(name)
             + " <br/> Vibe: module name not allowed " + vibe::srender::escape_html(name);
      }

      static std::string noSafe() noexcept {
        return "Vibe: error de sintaxis no <strong>'];'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>'];'</strong>  ";
      }

      static std::string noSafeData() noexcept {
        return "Vibe: error de sintaxis no <strong>']]'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>']]'</strong>  ";
      }
};

struct notify {
  static std::string noPath(const std::string& path) noexcept {
    return  "Can't get path file. '" + vibe::srender::escape_html(path) + "',  ";
  }
};

#endif // ! NOTIFY_HPP
