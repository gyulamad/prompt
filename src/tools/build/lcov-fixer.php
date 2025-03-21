#!/usr/bin/php
<?php

/**
 * This script supposed to fix all the bug came out from lcov:
 *  - ability to exclude files from lcov
 *  - excluding uncovered closing courly braces lines..
 * Run it right after lcov finished and before lcov starts with genhtml in your Makefile or whatever is your building process.
 * For e.g:
 *    lcov --no-external --directory . --capture --output-file coverage.info
 *    php lcov-fixer.php coverage.info src/NoNeedCoverage.cpp
 *    genhtml -s --demangle-cpp -o coverage coverage.info
 * 
 * usage:
 *    php lcov-fixer.php [coverage info file (optional, default: coverage.info)] [exluded files... (optional)]
 * 
 * example:
 *    php lcov-fixer.php coverage.info src/NoNeedCoverage.cpp
 */

echo "---=[ LCOV-FIXER ]=---\n";

$finfo = realpath($argv[1] ?? "coverage.info");
echo "Processing: $finfo\n";
$blocks = explode("\nend_of_record\n", file_get_contents($finfo));

for ($i = 2; $i < $argc; $i++) {
  $file = realpath($argv[$i]);
  if (!$file) {
    echo "\nERROR: File not found: {$argv[$i]}\n";
    exit(-1);
  }
  echo "Removing coverage info: $file\t";
  $results = [];
  foreach($blocks as $block) {
    $lines = explode("\n", $block);
    $remove = false;
    foreach ($lines as $line) {
      if (preg_match('/^SF:(.*)/', $line, $matches)) {
        $fname = realpath($matches[1]);
        if ($file === $fname) {
          echo ".";
          $remove = true;
          break;
        }
      }
    }
    if (!$remove) {
      $results[] = $block;      
    }
  }
  $blocks = $results;
  file_put_contents($finfo, implode("\nend_of_record\n", $blocks));  
  echo "\t[OK]\n";
}

echo "Removing uncovered closing courly braces lines...\n";
$nfolines = explode("\n", file_get_contents($finfo));
$currentfile = "";
foreach ($nfolines as $nfoline) {
  if (preg_match("/^DA:(\d+),0/", $nfoline, $matches)) { // uncovered line?
    $fcontent = explode("\n", file_get_contents($currentfile));
    if (trim($fcontent[$matches[1] - 1]) === "}") continue; // gotcha
  }
  if (preg_match("/^SF:(.*)$/", $nfoline, $matches)) {
    $currentfile = $matches[1];
  }
  $cleannfos[] = $nfoline;
}
file_put_contents($finfo, implode("\n", $cleannfos));


echo "All done. enjoy!\n";
