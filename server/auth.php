<?php

function formatted_now_date() : string {
    $date = new DateTime("now", new DateTimeZone("America/Los_Angeles"));
    return $date->format("Y-m-d h:i:s A T");
}

function authenticate() : bool {
    global $is_administrator;
    $curr_ver = "0.1";
    //Any users not included will not have access to the sector guard.
    //Change these:
    $administrators = [
        "User1", "User2"
    ];
    //While validation is case-insensitive, Do NOT set 2 users to have the same name even with different capitalization.
    //Change these:
    $valid_combos = [
        "User1" => "Password1",
	"User2" => "Password2",
	"User3" => "Password3"
    ];
    $usr = $_GET["usr"];
    $pwd = $_GET["pwd"];
    $ver = $_GET["ver"];
    if(!isset($usr) || !isset($pwd) || !isset($ver)) return false;
    if(strcmp($ver, $curr_ver)) return false;
    $checked = $valid_combos[$usr];
    $is_administrator = in_array($usr, $administrators);
    return isset($checked) && !strcmp($checked, $pwd);
}
?>
