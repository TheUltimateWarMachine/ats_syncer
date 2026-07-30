<?php
include("../auth.php");
const STORAGE_FILE = "storage.json";
const MAX_ANNOUNCEMENT_LEN = 64;
if (!authenticate()) {
    exit("-Invalid credentials!");
    
}
if ($_SERVER['REQUEST_METHOD'] == "GET") {
    $announcements = json_decode(file_get_contents(STORAGE_FILE));
    $n_announcements = strval(count($announcements));
    $client_last = $_GET["client_last"];
    if (isset($client_last) && strcmp($client_last, $n_announcements)) { //Display the newest announcement
        $last = end($announcements);
        echo "+" . $n_announcements . "\n" . $last->time . "\n" . $last->author . "\n" . $last->data;
    } else
        echo "-";
} else if ($_SERVER['REQUEST_METHOD'] == "POST") {
    //Add an announcement
    $data = file_get_contents("php://input");
    if ($data === "") {
        exit("-No data provided!");
        
    }
    if (!preg_match('/^[a-zA-Z0-9 .]+$/', $data)) {
        exit("-Data must only be alphanumeric! (spaces/periods allowed)");
        
    }
    if (strlen($data) > MAX_ANNOUNCEMENT_LEN) {
        exit("-Data is too long!");
        
    }
    $json = json_decode(file_get_contents(STORAGE_FILE));
    $date = new DateTime("now", new DateTimeZone("America/Los_Angeles"));
    $new_announcement = (object) [
        'time' => formatted_now_date(),
        'data' => $data,
        'author' => $_GET["usr"]
    ];
    $json[] = $new_announcement;
    file_put_contents(STORAGE_FILE, json_encode($json), LOCK_EX);
    echo "+";
}
?>