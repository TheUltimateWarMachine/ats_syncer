<?php
include("../auth.php");
const FILES_PER_SECTOR = 5;
if ($_SERVER["REQUEST_METHOD"] == "GET") {
    if(!authenticate()) {
        echo "-Invalid credentials!";
        exit();
    }
    echo "+";
    $dir = '../map';
    $groups = [];
    $c = 0;
    foreach (scandir($dir) as $file) {
        if ($file === '.' || $file === '..') {
            continue;
        }
        $path = $dir . '/' . $file;
        if (!is_file($path)) {
            continue;
        }
        $info = pathinfo($file);
        if (!isset($info['filename'], $info['extension'])) {
            continue;
        }
        $groups[$info['filename']][] = [
            'extension' => $info['extension'],
            'path' => $path,
        ];
        $c++;
    }
    echo strval($c / FILES_PER_SECTOR) . "\n";
    // Sort names alphabetically (optional).
    ksort($groups, SORT_STRING);
    foreach ($groups as $name => $files) {
        // Sort by extension alphabetically.
        usort($files, function ($a, $b) {
            return strcmp($a['extension'], $b['extension']);
        });
        echo $name . ":";
        foreach ($files as $file) {
            $hash = sha1_file($file['path']);
            echo "{$hash}/";
        }
        echo PHP_EOL;
    }
}
?>