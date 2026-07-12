LES DIFFERENTES ETAPES 

* Penser a partir des classes. On est plus en C. Regarde le main, trouve comment changer de vision afin de psasser de l'imperatif a l'oop. 
Prendre en compte le fait de developper les classes avant de coder. Tu vas devoir tout changer sinon. 
* Decomposer en sous probleme, creeer des classes pour resoudre ce sous probleme et faire remonter le tout. 

TODO : 
    - Finir de gérer les 
    - Creer la classe parsing_error qui prend des strings + le token pour montrer value et line

# 1 - INITIALISATION DU SERVEUR


DECLARATION DU SERVEUR

BIND DES PAGES D'ERREUR AUX PAGES CORRESPONDANTES :


## POUR LE FICHIER DE CONFIG

ON PARCOURT LE FICHIER DE CONFIG LIGNE PAR LIGNE : 

TOKENIZER :

3 DIFFERENTS ETATS :
    - GLOBAL
    - SERVER
    - LOCATION

( Créer une fonction isDirective() -> Check si type == *_DIRECTIVE )


REGLES POUR LE PARSING :
    Cas généraux d'erreur : 
    - Si token _type == *_DIRECTIVE && token next != VALUE 
    - Si token _type == SEMICOLON && token next != DIRECTIVE || CLOSE_BRACKET

GLOBAL DIRECTIVES
    Cas d'erreur state :
        - Si ETAT != GLOBAL

    - Si token == "server" 
        Cas d'erreur
            - Si etat != GLOBAL 
            - Si token next != OPEN_BRACKET 
            - Si token index != 0

        Sinon
            _state = SERVER;

SERVER DIRECTIVES
    Cas d'erreur state :
        - Si ETAT != SERVER

    - Si token == "listen"
        Cas d'erreur :
            - Si next token -> Not formatted (0 to 65535 || 0.0.0.0 to 255.255.255.255 + 0 to 65535)
            - Si next next token != SEMICOLON
            
        Sinon
            Creer socket;
                struct addrinfo hints, *res;
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                getaddrinfo("127.0.0.1", "8080", &hints, &res); || getaddrinfo(INADDR_ANY, "8080", &hints, &res)
                // res->ai_addr contains the sockaddr already filled with the right values
                freeaddrinfo(res);

    - Si token == "server_name"
        Cas d'erreur :
            - Si _name de server est déja set
            - Si next next token != SEMICOLON

        Sinon
            Server._name = next token

    - Si token == "client_max_body_size"
        Cas d'erreur :
            - Si next next token != SEMICOLON

        Sinon
            Server._max_body_size = next token

    - Si token == "error_page"
        Cas d'erreur :
            Si un token PARAMETER entre DIRECTIVE et dernier token PARAMETER != Nombre
            Si dernier PARAMETER n'est pas un chemin accessible

        Sinon
            rebinds la MapIntStr des pages d'erreur avec le dernier PARAMETER avant semicolon;

    - Si token == "location" 
        Cas d'erreur : 
            - si next next token != OPEN_BRACKET 
            - Si next token ne commence pas par / || Pas de . / .. 

        Sinon
            Créer location -> nom = next token;
            _state = LOCATION;

LOCATION DIRECTIVES
    Cas d'erreur state :
        - Si ETAT != LOCATION




Exemple de fichier de config :

### 1.1 NIVEAU SERVEUR :

server {
    # --- Top-level: bind targets ---
    listen 0.0.0.0:8080;          # one server block can listen on 
    listen 127.0.0.1:8081;        # several interface:port pairs
	-> Création d'un socket par listen trouvé -> (le stocker dans une map / vector ?)

    (server_name example.com;      # optional: only matters if you do virtual hosts) Est-ce qu'on host plusieurs serveurs ?


    client_max_body_size 10M;     # reject bodies bigger than this -> 413
	-> Sets SERVER -> SET la taille maximum d'un body au niveau du serveur (a convertir en octet ?), mais peut etre overwrite par chaque location (passer dans le constucteur des locations)

    error_page 404 /errors/404.html;
    error_page 500 502 503 504 /errors/5xx.html; (permet de lier plusieurs codes erreur a la meme page)
	-> Créer une map <int error, string path> pour pouvoir lier chaque code erreur a la page correspondante  /!\ (CREER UNE MAP AVANT LA LECTURE DU FICHIER DE CONFIG, RETOUCHER LES PATHS SI PATHS EXPLICITS VALIDES) /!\
		-> Verifier si les paths sont bien existants avant de faire le lien
			-> Ajouter si le fichier est bien existant
			-> Sinon servir la version hardcoded 


### 1.1 NIVEAU LOCATIONS :

    # --- Static site root ---
    location / {
        root      /var/www;       # URL /foo  ->  /var/www/foo
        -> set le membre path de location avec ce path si valide. Si invalide, renvoyer un erreur et ne pas initialiser le serveur, sauf si c'est le folder des error pages

        index     index.html;     # what to serve when the path is a directory
        -> Page a servir en cas de requete ciblant un dossier.
            -> S'il existe -> servir;
            -> S'il n'existe pas :
                -> Si autoindex on -> Générer le index.html
                -> Si autoindex off -> Return error 403

        methods   GET;            # GET only here
        -> Liste des methodes disponibles (stocker en tant que vector ?)


        autoindex off;            # no directory listing; 403/404 on a bare dir
        -> cf. un peu plus haut pour utilité
    }


    # --- A browsable directory ---
    PAREIL QU'AU DESSUS
    location /files {
        root      /var/www/files;
        methods   GET;
        autoindex on;             # generate an HTML listing when no index file
    }


    # --- Upload + delete endpoint ---
    location /uploads {
        root        /var/www/uploads;
        methods     GET POST DELETE;
        upload_dir  /var/www/uploads;   # where POSTed file bodies get written
        client_max_body_size 50M;       # override the server-wide cap here
        -> Redéfinit la taille maximale d'un body sur cette location uniquement
    }


    # --- CGI by extension ---
    location /cgi-bin {
        root    /var/www/cgi-bin;
        methods GET POST;
        cgi     .py  /usr/bin/python3;  # .py  -> run through python3
        cgi     .php /usr/bin/php-cgi;   # .php -> run through php-cgi
    }

    # --- A redirect ---
    location /old {
        return 301 /new;          # 301 + Location: /new, no body served
    }
    -> Gerer les redirections avec un path vers la nouvelle localisation.
        -> Ajouter un booleen redirigé ?
        -> Ajouter un booleen redirigé ?
}

CONFIG POUR LE SERVEUR
    socket
	server_name
	client max body size
	error_pages

CONFIG PAR LOCATIONS (CREATION D'UN OBJET DE CLASSE LOCATION PAR LOCATION TROUVEE)
	root
	methods
	index (used when the URL requested maps to a directory)
	autoindex (generates a list of the files in the folder, with links to each one and sent as a HTTP response) 
	upload_dir (where POSTed file bodies get written)
	client max body size (over_ride the cap for this location) (max_body_size has to be in each sub_class)
	cgi (use other languages)


Fonctions utilisees pour créer les sockets :
            socket -> Créer un fd au niveau de la page des fd (géré par le kernel)
			setsockopt -> Associe les options au socket_fd
			bind
			listen
			accept



# 2 - SERVEUR EN COURS D'EXECUTION
## 2.1 - REQUETES 



## 2.2 - REPONSES

