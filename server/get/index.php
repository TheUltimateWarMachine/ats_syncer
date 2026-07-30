<?php
include("../auth.php");
const FILE_ORDER = ["aux", "base", "data", "desc", "layer"];
if (!authenticate()) {
    echo "-Invalid credentials!";
    exit();
}

function encodeFiles(string $basePath, string $name, array $extensions): string
{
    $encoded = [];
    foreach ($extensions as $extension) {
        $filename = $basePath . $name . "." . $extension;
        $data = @file_get_contents($filename);
        if ($data === false) {
            exit("-Server is unable to read a part of sector $name!");
        }
        $encoded[] = base64_encode($data);
    }
    return implode('*', $encoded);
}

if($_SERVER['REQUEST_METHOD'] == 'GET') {
    $ACCEPTED_SECTORS = file("../sector_guard/storage.txt", FILE_IGNORE_NEW_LINES);
    $sector = $_GET["sector"];
    $sector = str_replace(' ', '+', $sector);
    if (!isset($sector)) {
        exit("-Sector unspecified!");
    }
    if (!in_array($sector, $ACCEPTED_SECTORS)) {
        exit("-Specified sector is outside of project bounds!");
    }
    echo "+" . encodeFiles("../map/", $sector, FILE_ORDER);
}

?>