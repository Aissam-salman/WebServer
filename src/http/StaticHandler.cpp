#include "StaticHandler.hpp"
#include "Parser.hpp"
#include "Response.hpp"
#include "utils.hpp"
#include <cstddef>
#include <dirent.h>
#include <fstream>
#include <ios>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// TROUVE LE MEILLEUR PATH POUR STOCKER DANS STATICHANDLER
Location &StaticHandler::findLocation(std::vector<Location> &locs,
                              const std::string &resource) {
  size_t best_len = 0;
  Location *best = NULL;
  for (size_t i = 0; i < locs.size(); i++) {
    const std::string &name = locs[i].getName();
    if (resource.find(name) == 0 && name.size() > best_len) {
      best = &locs[i];
      best_len = name.size();
    }
  }
  if (!best)
    throw std::runtime_error("404");
  return *best;
}

// BIND THE REQUEST AND RESOLVE ITS TARGET LOCATION
StaticHandler::StaticHandler(const Request &req, std::vector<Location> &locs)
    : _request(req), _location(findLocation(locs, req.getResource())) {}

// BUILD THE ON-DISK PATH: ROOT + RESOURCE MINUS THE LOCATION PREFIX
std::string StaticHandler::buildPath() const {
  std::string root = _location.getRootPath();
  std::string resource = _request.getResource();

  // enlever le prefix de la location du resource
  // ex: location /static/, resource /static/img.png → /img.png
  std::string loc_name = _location.getName();
  if (resource.find(loc_name) == 0)
    resource = resource.substr(loc_name.size());
  // s'assurer que root se termine par /
  if (!root.empty() && root[root.length() - 1] != '/')
    root += '/';
  // s'assurer que resource ne commence pas par /
  if (!resource.empty() && resource[0] == '/')
    resource = resource.substr(1);
  return root + resource;
}

// MIME TYPE ACCEPTED FOR AN UPLOAD, DERIVED FROM THE FILE EXTENSION
std::string
StaticHandler::getMimeTypeAllowForPost(const std::string &path) const {
  size_t dot = path.rfind('.'); // trouve le dernier '.' d'extention
  if (dot == std::string::npos)
    return "application/octet-stream";
  std::string ext = path.substr(dot); // recup l'extention
  if (ext == ".png")
    return "image/png";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".ico")
    return "image/x-icon";
  if (ext == ".txt")
    return "text/plain";
  if (ext == ".pdf")
    return "application/pdf";
  return "application/octet-stream";
}

// MIME TYPE FOR A RESPONSE, DERIVED FROM THE FILE EXTENSION
std::string StaticHandler::getMimeType(const std::string &path) const {
  size_t dot = path.rfind('.'); // trouve le dernier '.' d'extention
  if (dot == std::string::npos)
    return "application/octet-stream";
  std::string ext = path.substr(dot); // recup l'extention
  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".json")
    return "application/json";
  if (ext == ".png")
    return "image/png";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".ico")
    return "image/x-icon";
  if (ext == ".svg")
    return "image/svg+xml";
  if (ext == ".txt")
    return "text/plain";
  if (ext == ".pdf")
    return "application/pdf";

  return "application/octet-stream";
}

// READ A WHOLE FILE INTO A STRING (403 IF IT CANNOT BE OPENED)
std::string StaticHandler::readFile(const std::string &path) const {
  std::ifstream file(path.c_str(),
                     std::ios::binary); // std::ios::binary a voir si necessaire
                                        // pour image video etc.
  if (!file.is_open())
    throw std::runtime_error("403");
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// VERIFIE QUON VA PAS PLUS HAUT QUE ROOT
bool StaticHandler::isSafePath(const std::string &path) const {
  std::string root = _location.getRootPath();

  if (root.size() >= 2 && root[0] == '.' && root[1] == '/')
    root = root.substr(1); // enleve le '.' : "./www" -> "/www"
  if (root.empty() || root[0] != '/')
    root = "/" + root; // ajoute un / au debut : "www" -> "/www"
  if (root[root.length() - 1] != '/')
    root += '/'; // ajoute un / a la fin

#if DEBUG_RESPONSE == 1
  std::cout << "ROOT = " << root << endofline;
#endif

  std::vector<std::string> parts; // chaque segment du path /'...'/
  std::istringstream stream(path);
  std::string part;
  while (std::getline(stream, part, '/')) {
    if (part.empty() || part == ".") // dossier courant ou rien on skip
      continue;
    else if (part == "..") {
      if (!parts.empty())
        parts.pop_back(); // remonte dun niveau
      else
        return false; // quelqun veut me hacker
    } else
      parts.push_back(part); // tout va bien on lajoute
  }
  // reconstruit le path et vérifier
  std::string normalized = "/";
  for (size_t i = 0; i < parts.size(); ++i) {
    normalized += parts[i];
    normalized += "/";
  }
  // Vérifier que le chemin commence par root

#if DEBUG_RESPONSE == 1
  std::cout << "NORMALIZED = " << normalized << endofline;
#endif

  return normalized.find(root) == 0;
}

// GENERATE AN HTML DIRECTORY LISTING (AUTOINDEX)
std::string StaticHandler::generateAutoindex(const std::string &path) const {
  DIR *dir = opendir(path.c_str());
  if (!dir)
    throw std::runtime_error("403");
  // construit la liste  de fichiers dans le dossier bonus css jespere ca marche
  // lol
  std::ostringstream html;
  html << "<html><head><title>Index of " << _request.getResource() << "</title>"
       << "<style>body{font-family:monospace;padding:20px}"
       << "a{display:block;padding:2px 0}</style></head>"
       << "<body><h1>Index of " << _request.getResource() << "</h1><hr>";

  struct dirent *entry;
  while ((entry = readdir(dir))) // donne les infos du dosiey
  {
    std::string name = entry->d_name; // recup nom du fichier/dossier via la
                                      // struct systeme dirent
    if (name == ".")
      continue;
    html << "<a href=\"" << name;
    if (entry->d_type == DT_DIR) // d_type == type du repertoire(dossier ou
                                 // fichier), DT_DIR == cest un dossier ?
      html << "/";
    html << "\">" << name;
    if (entry->d_type == DT_DIR)
      html << "/";
    html << "</a>";
  }
  closedir(dir);
  html << "<hr></body></html>";
  return html.str();
}

typedef std::map<std::string, std::string>::const_iterator headerIter;

// TRUE IF THE UPLOAD'S DECLARED TYPE MATCHES ITS EXTENSION
bool StaticHandler::isSafeFile(std::string &file_path,
                               std::string &file_type) const {
  std::string mimetype = this->getMimeTypeAllowForPost(file_path);
#if DEBUG == 1
  debug(file_path, "file_path", RED);
  debug(file_type, "filetype", RED);
  debug(mimetype, "mimetype", RED);
#endif
  if (mimetype == file_type && mimetype != "application/octet-stream")
    return true;
  return false;
}

// TRUE IF THE TARGET FILE DOES NOT ALREADY EXIST (READABLE)
bool StaticHandler::isFileAlreadyExist(std::string &file_path) const {
  if (access(file_path.c_str(), R_OK) == 0)
    return false;
  return true;
}

typedef std::map< std::string, std::string>::const_iterator mapStrIter;

// DISPATCH THE REQUEST: REDIRECT, METHOD CHECK, THEN SERVE / UPLOAD / DELETE
Response StaticHandler::handle() const {
  // redirection configurée dans la location

  if (_location.getReturn()) {
    Response resp(_location.getReturnErrorCode(), "",
                  ""); // construit la reponse avec le code de redir
    resp.setHeader(
        "Location",
        _location.getReturnPath()); // met le header obligatoire dune redir
    return resp;
  }
  // vérifier que la méthode est autorisée pour cette location (bitmask)
  std::map<std::string, e_methods>::const_iterator mit =
      getMethodMap().find(_request.getMethod());
  if (mit == getMethodMap().end() ||
      (_location.getMethodFlag() & mit->second) == 0)
    throw std::runtime_error("405");

  // construire et vérifier le path
  std::string path = buildPath();

  // #if DEBUG_RESPONSE == 1
  //   std::cout << BOLD_CYAN << "PATH RESPONSE = " << path << endofline;
  // #endif


  if (!isSafePath(path))
    throw std::runtime_error("403");

  // #if DEBUG_RESPONSE == 1
  //   std::cout << BOLD_CYAN << "JE PASSE ICI" << endofline;
  // #endif

  struct stat st;
  if (stat(path.c_str(), &st) != 0) // recup les stats du fichier
  {
    if (errno == EACCES)
      throw std::runtime_error("403"); // pas les droits
    throw std::runtime_error("404");   // pas trouvee
  }
  // c'est un dossier
  if (S_ISDIR(st.st_mode)) {
    /////////////////////////////////////////////////////////
    if (_request.getMethod() == "POST") {
      mapStrIter header_content = _request.getHeaders().find("Content-Type");
      if (header_content == _request.getHeaders().end())
          return Response(400, "Bad Request");
      size_t header_content_type_pos = header_content->second.find("multipart/form-data");

      // accept just multipart/form-data for POST
      if (header_content_type_pos != std::string::npos) {
        std::string boundary =
            extractBoundary(header_content->second);

        if (boundary.empty())
          return Response(400, "Bad Request");

        // recup filename,
        size_t start_pos = _request.getBody().find(boundary);
        
        std::string filename;
        std::string file_type;
        size_t end;
        // INFO: I don't handle multiple files uploads but it's fine, not for
        // this night
        while (start_pos != std::string::npos) {
          size_t filename_pos =
              _request.getBody().find("filename=", start_pos + boundary.size());
          if (filename_pos == std::string::npos)
            return Response(400, "Bad Request");

          size_t start_filename =
              _request.getBody().find("\"", filename_pos + 8);

          size_t end_filename = 0;
          if (start_filename != std::string::npos) {
            end_filename = _request.getBody().find("\"", start_filename + 1);
          }

          if (end_filename == 0)
            return Response(400, "Bad Request");

          filename = _request.getBody().substr(
              start_filename + 1, end_filename - start_filename - 1);

#if DEBUG == 1
          debug(filename, "filename_value", BOLD_GREEN);
#endif
          if (!filename.empty()) {
            size_t start =
                _request.getBody().find("Content-Type: ", end_filename);
            if (start == std::string::npos)
              return Response(400, "Bad Request");
            end = _request.getBody().find("\r\n", start);
            if (end == std::string::npos)
              return Response(400, "Bad Request");
            file_type = _request.getBody().substr(start + 14, end - start - 14);
            break;
          }
          start_pos =
              _request.getBody().find(boundary, start_pos + boundary.size());
        }

        std::string file_path = _location.getRootPath() + "/" + filename;

        if (!isSafePath(file_path))
          return Response(400, "Bad Request");

        if (!isSafeFile(file_path, file_type))
          return Response(400, "Bad Request");

        if (!isFileAlreadyExist(file_path))
          return Response(500, "Already exist");

        size_t start_file = _request.getBody().find("\r\n\r\n", end);
        size_t end_file = _request.getBody().find(boundary, start_file);
        std::string file_content = _request.getBody().substr(start_file + 4, end_file - start_file - 8);

#if DEBUG == 1
        debug(start_file, "start", YELLOW);
        debug(end_file, "end", YELLOW);
#endif

        std::ofstream of;
        of.open(file_path.c_str(), std::ios_base::binary);
        if (of.is_open()) {
            of << file_content;
            of.close();
        } else {
          return Response(500, "Cannot upload new file");
        }
        return Response(201, "Created");
      }
      return Response(400, "Bad Request");
    }
    ///////////////////////////////////////////////////

    // chercher l'index
    std::string index_path = path;
    if (index_path[index_path.length() - 1] !=
        '/') // mettre un / a la fin si ya pas ex: "/doc/file + /"
      index_path += '/';
    index_path +=
        _location.getIndexPath().empty() // si pas de config mettre "index.html"
                                         // mais je crois pas besoin lol
            ? "index.html"
            : _location.getIndexPath();
    struct stat ist;
    if (stat(index_path.c_str(), &ist) == 0 &&
        S_ISREG(
            ist.st_mode)) // regarde si ce quon a choper est bien fichier valid
    {
      if (access(index_path.c_str(), R_OK) !=
          0) // si lisible reponse 200 sinon 403 pas les droits
        throw std::runtime_error("403");
      return Response(200, readFile(index_path), getMimeType(index_path));
    }
    // pas d'index donc autoindex ou pas ?si oui genere sinon 403
    if (_location.getAutoIndex())
      return Response(200, generateAutoindex(path), "text/html");
    throw std::runtime_error("403");
  }
  // c'est un fichier normal (question du DELETE comment gerer(pas de fonction
  // dans le sujet?))
  else if (S_ISREG(st.st_mode)) {
    if (_request.getMethod() == "DELETE") {
      if (access(path.c_str(), W_OK) != 0)
        throw std::runtime_error("403");
      if (std::remove(path.c_str()) !=
          0) // std::remove == unlink detruit le fichier
      {
        if (errno == EACCES || errno == EPERM) // pas le droit de remove
          throw std::runtime_error("403");
        throw std::runtime_error("500");
      }
      return Response(204, "",
                      ""); // a delete successfully, pas de type mime ou body
    }
    if (access(path.c_str(), R_OK) != 0)
      throw std::runtime_error("403");
    return Response(200, readFile(path), getMimeType(path));
  }
  // ni fichier ni dossier (socket, pipe...) 404
  throw std::runtime_error("404");
}
