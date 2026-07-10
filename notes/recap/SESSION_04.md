# Session 04 — 2026-07-09

## Chunked Transfer-Encoding — état des lieux

Point de départ : on gère pas du tout les requêtes chunked. Vérifié par grep sur tout le repo (`server/`, `utils/`) : **zéro occurrence** de `chunk` ou `Transfer-Encoding` dans le code source. C'est un vrai trou, pas juste un oubli mineur.

### Ce qui existe déjà (à relire avant de coder)

- `docs/06_FRAMING.md` — a une esquisse de fonction `decode_chunked(SocketBuffer& sock)` (pseudocode, pas du code réel)
- `docs/14_CGI.md` (lignes ~153, ~195-203) — parle du besoin de dé-chunker le body **avant** de l'envoyer au CGI, parce que `CONTENT_LENGTH` doit être un nombre exact envoyé au script
- `docs/02_MESSAGE_ANATOMY.md` (~147-148), `docs/01_FUNDAMENTALS.md` (~76, ~85-90) — notions générales Content-Length vs chunked
- `tests/postman/webserv.postman_collection.json` (~498-512, ~1784) — test existant nommé **"GAP: POST sans Content-Length -> 411 attendu (pas de chunked supporté)"**. Ce test documente le comportement actuel (incorrect) et devra changer de comportement attendu une fois le chunked géré.

### Les 3 endroits qui bloquent concrètement

1. **`server/client/Client.cpp:63-78` — `Client::isRequestCompleted()`**
   Ne regarde que `Content-Length` (recherche par sous-chaîne naïve `"Content-Length:"` dans tout `_buffer_read`, pas un vrai lookup de header — attention si le body contient ce texte). Si pas de `Content-Length` trouvé → retourne `true` dès que les headers sont là (ligne 77). Donc une requête chunked (qui n'a pas de `Content-Length`) est actuellement considérée "complète" juste après les headers, avant même d'avoir lu le body. `Transfer-Encoding` n'est jamais regardé.

2. **`server/Request.cpp:63-69` — `Request::parseRequest()`**
   Si `body.length() > 1` et pas de `Content-Length` → throw `"411"`. Une requête chunked a un body mais pas de `Content-Length` → elle se fait rejeter directement avec ce code actuel.

3. **Aucune logique de décodage chunked nulle part.** Format à décoder : `<taille-hex>\r\n<data>\r\n` répété, terminé par `0\r\n\r\n` (+ trailers optionnels). Pas de state machine de parsing incrémental côté `Request` — `parseRequest()` est one-shot, appelé une seule fois quand `isRequestCompleted()` dit que tout est là (`Server.cpp` ~265).

### Impact en aval (une fois le décodage fait)

- `Request::parseCgi_env()` (`Request.cpp:110`) lit `Content-Length` dans les headers pour construire `CONTENT_LENGTH` du CGI — il faudra le recalculer à partir de la taille réelle du body décodé si la requête était chunked à l'origine.
- `StaticHandler.cpp` (upload multipart, ~lignes 266-355) travaille sur `_request.getBody()` en supposant que c'est déjà le contenu brut complet. Si le body arrive chunked et n'est pas décodé avant, tout le parsing multipart (recherche de boundary, filename, etc.) part en vrille.
- Détail annexe repéré en passant (pas lié au chunked mais à surveiller si on touche cette zone) : `StaticHandler.cpp` fait un `_request.getHeaders().find("Content-Type")->second` sans vérifier `!= end()` → UB potentiel si le header est absent.

### Questions à trancher avant de coder

- Où détecter `Transfer-Encoding: chunked` — dans `Client` (pendant la lecture socket) ou dans `Request` (après réception complète) ?
- Le décodage doit-il être incrémental (au fur et à mesure des `recv()`) ou fait d'un coup sur le buffer accumulé ? Vu que `parseRequest()` est déjà one-shot, la deuxième option colle mieux à l'architecture actuelle — mais alors comment `isRequestCompleted()` sait qu'on a atteint le chunk terminal `0\r\n\r\n` sans dupliquer la logique de parsing ?
- Une fois décodé : où stocker le body décodé et le "Content-Length" recalculé, pour que `parseCgi_env()` et `StaticHandler` n'aient pas besoin de savoir que c'était chunké à l'origine ?
- Faut-il gérer les trailers (headers après le dernier chunk) ? (Probablement pas nécessaire pour le scope du projet 42, mais à vérifier dans le sujet.)

### Prochaine étape suggérée

Relire `docs/06_FRAMING.md` et `docs/14_CGI.md` en détail, puis attaquer le point 1 (détection dans `isRequestCompleted`) avant le décodage lui-même — tant que la détection de "requête complète" n'est pas correcte pour le chunked, le reste ne peut pas être testé proprement.


curl -v -X POST http://localhost:8090/uploads \
  -H "Transfer-Encoding: chunked" \
  -F "file=@$HOME/Pictures/test.png"
