#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <string>

#include "secure_render.h"

// Names/paths reflected into a response body must be HTML-escaped and
// stripped of control characters: an unescaped name is an XSS sink when
// the route serves text/html.

struct notify_html {
      static std::string noPath() noexcept {
        return "Vermell: error no se puedo encontrar la ruta! <br/> Vermell: error cant get the path! ";
      }
      static std::string noFIle(const std::string& name) noexcept {
        return "Vermell: error no se puedo encontrar el archivo " + vermell::srender::escape_html(name)
             + " <br/> Vermell: error cant get the file  " + vermell::srender::escape_html(name);
      }

      static std::string badName(const std::string& name) noexcept {
        return "Vermell: nombre de modulo no permitido " + vermell::srender::escape_html(name)
             + " <br/> Vermell: module name not allowed " + vermell::srender::escape_html(name);
      }

      static std::string noSafe() noexcept {
        return "Vermell: error de sintaxis no <strong>'];'</strong>  <br/> Vermell: sintax error whiout closing the tag with <strong>'];'</strong>  ";
      }

      static std::string noSafeData() noexcept {
        return "Vermell: error de sintaxis no <strong>']]'</strong>  <br/> Vermell: sintax error whiout closing the tag with <strong>']]'</strong>  ";
      }
};

struct notify {
  static std::string noPath(const std::string& path) noexcept {
    return  "Can't get path file. '" + vermell::srender::escape_html(path) + "',  ";
  }
};

#endif // ! NOTIFY_HPP
