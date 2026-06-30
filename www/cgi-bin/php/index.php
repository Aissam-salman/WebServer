#!/usr/bin/php-cgi
<?php
header_remove('X-Powered-By');

function connectionDb(string $dbPath): SQLite3
{
	$db = new SQLite3($dbPath);
	$db->exec("CREATE TABLE IF NOT EXISTS images (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		filename TEXT NOT NULL,
		size INTEGER NOT NULL,
		path TEXT NOT NULL,
		created_at TEXT NOT NULL,
		updated_at TEXT NOT NULL
		)");
	return $db;
}

function jsonResponseErr(int $statusCode, string $msg): never
{
	http_response_code($statusCode);
	echo json_encode(["error" => $msg]);
	exit;
}

function mapRawToArray(array $row): array
{
	return [
		"id" => (int)$row["id"],
		"filename" => $row["filename"],
		"size" => (int)$row["size"],
		"path" => $row["path"],
		"created_at" => $row["created_at"],
		"updated_at" => $row["updated_at"],
	];
}

// ROUTER 
/**
 * CGI PHP - Catalogue d'images
 * Routes (via PATH_INFO):
 *   GET    /                -> liste toutes les images
 *   GET    /{id}            -> details d'une image
 *   POST   /                -> upload (multipart/form-data OU body brut)
 *   PUT    /{id}            -> remplace le fichier d'une image existante
 *   DELETE /{id}            -> supprime une image (fichier + DB)
 */
//
function Get(SQLite3 $db, ?int $id): void
{
	if ($id !== null) {
		$query = $db->prepare("SELECT * FROM images WHERE id = :id");
		$query->bindValue(":id", $id, SQLITE3_INTEGER);
		$imgRow = $query->execute()->fetchArray(SQLITE3_ASSOC);
		if (!$imgRow) {
			jsonResponseErr(404, "Not found");
		}
		http_response_code(200);
		echo json_encode(mapRawToArray($imgRow));
		return;
	}
	$imgsRows = $db->query("SELECT * FROM images");
	$imgs = [];
	while ($row = $imgsRows->fetchArray(SQLITE3_ASSOC)) {
		$imgs[] = mapRawToArray($row);
	}
	http_response_code(200);
	echo json_encode($imgs);
}

function Post(SQLite3 $db, string $storageDir, array $imgExt): void
{
	$contentType = $_SERVER["CONTENT_TYPE"] ?? "";
	if (str_starts_with($contentType, "multipart/form-data")) {
		if (empty($_FILES["image"]) || $_FILES["image"]["error"] !== UPLOAD_ERR_OK) {
			jsonResponseErr(400, "No valid file uploaded");
		}

		$file = $_FILES["image"];
		$mimeType = mime_content_type($file["tmp_name"]);

		if (!isset($imgExt[$mimeType])) {
			jsonResponseErr(415, "Unsupported content type");
		}

		$extension = $imgExt[$mimeType];
		$filename = uniqid("img_", true) . "." . $extension;
		$filepath = $storageDir . $filename;
		if (!move_uploaded_file($file["tmp_name"], $filepath)) {
			jsonResponseErr(500, "Failed to move uploaded file");
		}
		$size = $file["size"];
	} else {
		$bodyRaw = file_get_contents("php://input");
		if (empty($bodyRaw)) {
			jsonResponseErr(400, "Empty body");
		}

		if (!isset($imgExt[$contentType])) {
			jsonResponseErr(415, "Unsupported content type: " . $contentType);
		}
		$extension = $imgExt[$contentType];
		$filename = uniqid("img_", true) . "." . $extension;
		$filepath = $storageDir . $filename;

		$size = file_put_contents($filepath, $bodyRaw);
		if ($size == false) {
			jsonResponseErr(500, "Failed to move uploaded file");
		}
	}

	$now = date("Y-m-d H:i:s");
	$query = $db->prepare("INSERT INTO images (filename, size, path, created_at, updated_at) VALUES (:filename, :size, :path, :created_at, :updated_at)");
	$query->bindValue(":filename", $filename, SQLITE3_TEXT);
	$query->bindValue(":size", $size, SQLITE3_INTEGER);
	$query->bindValue(":path", $filepath, SQLITE3_TEXT);
	$query->bindValue(":created_at", $now, SQLITE3_TEXT);
	$query->bindValue(":updated_at", $now, SQLITE3_TEXT);
	$query->execute();

	$id = $db->lastInsertRowID();
	http_response_code(201);
	echo json_encode([
		"id" => $id,
		"filename" => $filename,
		"size" => $size,
		"path" => $filepath,
		"created_at" => $now,
		"updated_at" => $now
	]);
}

function Put(SQLite3 $db, string $storageDir, array $imgExt, ?int $id): void
{
	if ($id == null) {
		jsonResponseErr(400, "Missing image id");
	}

	$req = $db->prepare("SELECT * FROM images WHERE id = :id");
	$req->bindValue(":id", $id, SQLITE3_INTEGER);
	$row = $req->execute()->fetchArray(SQLITE3_ASSOC);
	if (!$row) {
		jsonResponseErr(404, "Not found");
	}

	$contentType = $_SERVER["CONTENT_TYPE"] ?? "";
	$bodyRaw = file_get_contents("php://input");
	if (empty($bodyRaw)) {
		jsonResponseErr(400, "Empty body");
	}
	if (!isset($imgExt[$contentType])) {
		jsonResponseErr(415, "Unsupported content type: " . $contentType);
	}

	$filepathOld = $row["filepath"];
	if (file_exists($filepathOld)) {
		unlink($filepathOld);
	}

	$extension = $imgExt[$contentType];
	$filename = uniqid("img_", true) . "." . $extension;
	$filepath = $storageDir . $filename;

	$size = file_put_contents($filepath, $bodyRaw);
	if ($size == false) {
		jsonResponseErr(500, "Failed to move uploaded file");
	}

	$now = date("Y-m-d H:i:s");
	$req = $db->prepare("UPDATE images filename = :filename, size = :size, path = :path, created_at = :created_at, updated_at = :updated_at WHERE id = :id");
	$req->bindValue(":filename", $filename, SQLITE3_TEXT);
	$req->bindValue(":size", $size, SQLITE3_INTEGER);
	$req->bindValue(":path", $filepath, SQLITE3_TEXT);
	$req->bindValue(":created_at", $now, SQLITE3_TEXT);
	$req->bindValue(":updated_at", $now, SQLITE3_TEXT);
	$req->bindValue(":id", $id);
	$req->execute();

	http_response_code(200);
	echo json_encode([
		"id" => $id,
		"filename" => $filename,
		"size" => $size,
		"path" => $filepath,
		"created_at" => $row["created_at"],
		"updated_at" => $now
	]);
}

function Delete(SQLite3 $db, string $storageDir, ?int $id): void
{
	if ($id == null) {
		jsonResponseErr(400, "Missing image id");
	}
	$req = $db->prepare("SELECT * FROM images WHERE id = :id");
	$req->bindValue(":id", $id, SQLITE3_INTEGER);
	$row = $req->execute()->fetchArray(SQLITE3_ASSOC);
	if (!$row) {
		jsonResponseErr(404, "Not found");
	}

	$filepathOld = $row["filepath"];
	if (file_exists($filepathOld)) {
		unlink($filepathOld);
	}
	$req = $db->prepare("DELETE FROM images WHERE id = :id");
	$req->bindValue(":id", $id, SQLITE3_INTEGER);
	$req->execute();

	http_response_code(204);
}

function main(): void
{
	$storageDir = "../../../www/img/";
	$dbPath = "catalog.db";

	$imgExt = [
		"image/jpeg" => "jpg",
		"image/png" => "png",
		"image/webp" => "webp",
	];

	if (!is_dir($storageDir)) {
		mkdir($storageDir, 0755, true);
	}

	$pathInfo = getenv("PATH_INFO") ?: "/";
	$params = array_values(array_filter(explode("/", $pathInfo)));
	$id = isset($params[0]) && ctype_digit($params[0]) ? (int)$params[0] : null;
	$methodHtpp = $_SERVER["REQUEST_METHOD"] ?? "GET";

	$db = connectionDb($dbPath);

	switch ($methodHtpp) {
		case "GET":
			Get($db, $id);
			break;
		case "POST":
			Post($db, $storageDir, $imgExt);
			break;
		case "PUT":
			Put($db, $storageDir, $imgExt, $id);
			break;
		case "DELETE":
			Delete($db, $storageDir, $id);
		default:
			jsonResponseErr(405, "Method not allowed");
	}

	$db->close();
}
header("Content-Type: application/json");
main();
