#include "StaticHandler.hpp"
#include "Parser.hpp"
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>


//trouve le meilleur path pour stocker dans statichandler
static Location& findLocation(std::vector<Location>& locs, const std::string& resource)
{
    size_t best_len = 0;
    Location* best  = NULL;
    for (size_t i = 0; i < locs.size(); i++) {
        const std::string& name = locs[i].getName();
        if (resource.find(name) == 0 && name.size() > best_len) {
            best     = &locs[i];
            best_len = name.size();
        }
    }
    if (!best)
        throw std::runtime_error("404");
    return *best;
}

StaticHandler::StaticHandler(const Request& req, std::vector<Location>& locs)
    : _request(req), _location(findLocation(locs, req.getResource())) {}


std::string StaticHandler::buildPath() const
{
    std::string root     = _location.getRootPath();
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

std::string     StaticHandler::getMimeType(const std::string& path) const
{
    size_t dot = path.rfind('.'); //trouve le dernier '.' d'extention
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dot);//recup l'extention
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")                   return "text/css";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".json")                  return "application/json";
    if (ext == ".png")                   return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")                   return "image/gif";
    if (ext == ".ico")                   return "image/x-icon";
    if (ext == ".svg")                   return "image/svg+xml";
    if (ext == ".txt")                   return "text/plain";
    if (ext == ".pdf")                   return "application/pdf";
    
    return "application/octet-stream";
}

std::string     StaticHandler::readFile(const std::string& path) const
{
    std::ifstream file(path.c_str(), std::ios::binary); //std::ios::binary a voir si necessaire pour image video etc.
    if (!file.is_open())
        throw std::runtime_error("403");
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

//verifie quon va pas plus haut que root
bool StaticHandler::isSafePath(const std::string& path) const
{
    std::string root = _location.getRootPath();    


    if (root.size() >= 2 && root[0] == '.' && root[1] == '/')
        root = root.substr(1);                    // enleve le '.' : "./www" -> "/www"
    if (root.empty() || root[0] != '/')
        root = "/" + root;                        // ajoute un / au debut : "www" -> "/www"
    if (root[root.length() - 1] != '/')
        root += '/';                              // ajoute un / a la fin

# if DEBUG_RESPONSE == 1
    std::cout << "ROOT = " << root << endofline;
#endif

    std::vector<std::string> parts; //chaque segment du path /'...'/
    std::istringstream stream(path);
    std::string part;
    while (std::getline(stream, part, '/'))
    {
        if (part.empty() || part == ".") //dossier courant ou rien on skip
            continue;
        else if (part == "..")
        {
            if (!parts.empty())
                parts.pop_back(); //remonte dun niveau 
            else
                return false; //quelqun veut me hacker
        }
        else
            parts.push_back(part);//tout va bien on lajoute
    }
    //reconstruit le path et vérifier
    std::string normalized = "/";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        normalized += parts[i];
        normalized += "/";
    }
    // Vérifier que le chemin commence par root

# if DEBUG_RESPONSE == 1
    std::cout << "NORMALIZED = " << normalized << endofline;
#endif

    return normalized.find(root) == 0;
}

std::string StaticHandler::generateAutoindex(const std::string& path) const
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
        throw std::runtime_error("403");
    //construit la liste  de fichiers dans le dossier bonus css jespere ca marche lol
    std::ostringstream html;
    html << "<html><head><title>Index of "
         << _request.getResource() << "</title>"
         << "<style>body{font-family:monospace;padding:20px}"
         << "a{display:block;padding:2px 0}</style></head>"
         << "<body><h1>Index of " << _request.getResource() << "</h1><hr>";

    struct dirent* entry;
    while ((entry = readdir(dir))) //donne les infos du dosiey
    {
        std::string name = entry->d_name; //recup nom du fichier/dossier via la struct systeme dirent
        if (name == ".")
            continue;
        html << "<a href=\"" << name;
        if (entry->d_type == DT_DIR) //d_type == type du repertoire(dossier ou fichier), DT_DIR == cest un dossier ?
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

Response StaticHandler::handle() const
{
    //redirection configurée dans la location

    if (_location.getReturn())
    {
        Response resp(_location.getReturnErrorCode(), "", "");  //construit la reponse avec le code de redir
        resp.setHeader("Location", _location.getReturnPath()); //met le header obligatoire dune redir
        return resp;
    }
    // vérifier que la méthode est autorisée pour cette location (bitmask)
    std::map<std::string, e_methods>::const_iterator mit =
        getMethodMap().find(_request.getMethod());
    if (mit == getMethodMap().end() || (_location.getMethodFlag() & mit->second) == 0)
        throw std::runtime_error("405");

    //construire et vérifier le path
    std::string path = buildPath();
# if DEBUG_RESPONSE == 1
    std::cout << BOLD_CYAN << "PATH RESPONSE = " << path << endofline;
#endif
    if (!isSafePath(path))
        throw std::runtime_error("403");

# if DEBUG_RESPONSE == 1
    std::cout << BOLD_CYAN << "JE PASSE ICI" << endofline;
#endif

    struct stat st;
    if (stat(path.c_str(), &st) != 0) //recup les stats du fichier
    {
        if (errno == EACCES)
            throw std::runtime_error("403"); //pas les droits
        throw std::runtime_error("404"); //pas trouvee
    }
    //c'est un dossier
    if (S_ISDIR(st.st_mode))
    {
        //INFO: is POST methods        //
        // chercher l'index
        std::string index_path = path;
        if (index_path[index_path.length() - 1] != '/') // mettre un / a la fin si ya pas ex: "/doc/file + /"
            index_path += '/';
        index_path += _location.getIndexPath().empty() // si pas de config mettre "index.html" mais je crois pas besoin lol
                      ? "index.html"
                      : _location.getIndexPath();
        struct stat ist;
        if (stat(index_path.c_str(), &ist) == 0 && S_ISREG(ist.st_mode)) // regarde si ce quon a choper est bien fichier valid
        {
            if (access(index_path.c_str(), R_OK) != 0) //si lisible reponse 200 sinon 403 pas les droits
                throw std::runtime_error("403");
            return Response(200, readFile(index_path), getMimeType(index_path));
        }
        // pas d'index donc autoindex ou pas ?si oui genere sinon 403
        if (_location.getAutoIndex())
            return Response(200, generateAutoindex(path), "text/html");
        throw std::runtime_error("403");
    }
    //c'est un fichier normal (question du DELETE comment gerer(pas de fonction dans le sujet?))
    if (S_ISREG(st.st_mode))
    {
        if (_request.getMethod() == "DELETE")
        {
            if (access(path.c_str(), W_OK) != 0)
                throw std::runtime_error("403");
            if (std::remove(path.c_str()) != 0) //std::remove == unlink detruit le fichier
            {
                if (errno == EACCES || errno == EPERM) //pas le droit de remove
                    throw std::runtime_error("403");
                throw std::runtime_error("500");
            }
            return Response(204, "", ""); //a delete successfully, pas de type mime ou body
        }
        if (access(path.c_str(), R_OK) != 0)
            throw std::runtime_error("403");
        return Response(200, readFile(path), getMimeType(path));
    }
    //ni fichier ni dossier (socket, pipe...) 404
    throw std::runtime_error("404");
}
