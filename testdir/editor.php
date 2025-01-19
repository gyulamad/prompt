<?php

function testFunction() {
  echo "A teszt függvény lefutott.\n";
}

$input = explode(" ", $_SERVER['argv'][1]);

if (isset($input[0])) {
    switch ($input[0]) {
        case "test":
            testFunction();
            break;
        default:
            echo "Ismeretlen parancs.\n";
    }
} else {
    echo "Nincs parancs megadva.\n";
}

?>
