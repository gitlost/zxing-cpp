<?php
/*
* Copyright 2023 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

// Generate `aiInfos[]` for `HRIFromGS1()` ("core/src/HRI.cpp")

// To run (from project directory):
//
//   php core/tools/gen_ai_infos.php
//
// and paste output into "core/src/HRI.cpp", replacing `aiInfos[]` def

$basename = basename(__FILE__);

$opts = getopt('f:t:');

$file = isset($opts['f']) ? $opts['f'] : '';
$tab = isset($opts['t']) ? $opts['t'] : "\t";

$tag = '';
if ($file === '') {
    // Get latest released version
    $cmd = 'git ls-remote --tags --sort=v:refname https://github.com/gs1/gs1-syntax-dictionary';
    if (($out = exec($cmd)) === false) {
        exit("$basename:" . __LINE__ . " ERROR: Could not execute command \"$cmd\"" . PHP_EOL);
    }
    $tag = substr($out, -10);
    if (!preg_match('/^[0-9]{4}-[0-9]{2}-[0-9]{2}$/', $tag)) {
        exit("$basename:" . __LINE__ . " ERROR: Could not recognize tag \"$tag\"" . PHP_EOL);
    }
    $file = "https://raw.githubusercontent.com/gs1/gs1-syntax-dictionary/$tag/gs1-syntax-dictionary.txt";
}

if (($get = file_get_contents($file)) === false) {
    exit("$basename:" . __LINE__ . " ERROR: Could not read file \"$file\"" . PHP_EOL);
}

$lines = explode("\n", $get);

$ais_maxs = $sort_ais = array();

// Parse the lines into AIs and maximum lengths
$line_no = 0;
foreach ($lines as $line) {
    $line_no++;
    if ($line === '') {
        continue;
    }
    if ($line[0] === '#') {
        // If using local file, set tag
        if ($tag === '' && preg_match('/^# Release: ([0-9]{4}-[0-9]{2}-[0-9]{2})$/', $line, $matches)) {
            $tag = $matches[1];
        }
        continue;
    }
    if (!preg_match('/^([0-9]+(?:-[0-9]+)?) +([ *] )([NXYZ][0-9.][ NXYZ0-9.,a-z=|\[\]]*)(?:# (.+))?$/', $line, $matches)) {
        print $line . PHP_EOL;
        exit("$basename:" . __LINE__ . " ERROR: Could not parse line $line_no" . PHP_EOL);
    }
    $ai = $matches[1];
    $spec = preg_replace('/ +req=[0-9,n]*/', '', trim($matches[3])); // Strip mandatory association info
    $spec = preg_replace('/ +ex=[0-9,n]*/', '', $spec); // Strip invalid pairings info
    $spec = preg_replace('/ +dlpkey[=0-9,|]*/', '', $spec); // Strip Digital Link primary key info

    if (($hyphen = strpos($ai, '-')) !== false) {
        $ai_s = (int) substr($ai, 0, $hyphen);
        $ai_e = (int) substr($ai, $hyphen + 1);
        $ais = array($ai_s, $ai_e);
        $sort_ais[] = $ai_s;
    } else {
        $ai = (int) $ai;
        $ais = $ai;
        $sort_ais[] = $ai;
    }

    $min = $max = 0;
    $parts = explode(' ', $spec);
    foreach ($parts as $part) {
        $checkers = explode(',', $part);
        $validator = array_shift($checkers);
        if (preg_match('/^([NXYZ])([0-9]+)?(\.\.[0-9|]+)?$/', $validator, $matches)) {
            if (count($matches) === 3) {
                $min += (int) $matches[2];
                $max += (int) $matches[2];
            } else {
                $min += $matches[2] === '' ? 1 : (int) $matches[2];
                $max += (int) substr($matches[3], 2);
            }
        } else if (preg_match('/^\[([NXYZ])([1-9]+)?(\.\.[0-9|]+)?\]$/', $validator, $matches)) {
            if (count($matches) === 3) {
                $min += 0;
                $max += (int) $matches[2];
            } else {
                $min += $matches[2] === '' ? 0 : (int) $matches[2];
                $max += (int) substr($matches[3], 2);
            }
        } else {
            exit("$basename:" . __LINE__ . " ERROR: Could not parse validator \"$validator\" line $line_no" . PHP_EOL);
        }
    }
    if ($min === $max) {
        $ais_maxs[] = array($ais, $max, false /*used flag*/);
    } else {
        $ais_maxs[] = array($ais, -$max, false /*used flag*/);
    }
}
array_multisort($sort_ais, $ais_maxs);

// Helper to print out `aiInfos[]` entries
function print_ai($tab, $ais, $max, $from, $limit, &$prev_unit, $unit, $used = false) {
    if (is_array($ais)) {
        if ($ais[0] >= $from && $ais[0] < $limit && !$used) {
            if ($ais[1] >= $prev_unit) {
                print "\n";
                $prev_unit = ($ais[1] * $unit) / $unit + $unit;
            }
            for ($ai = $ais[0]; $ai <= $ais[1]; $ai++) {
                printf("{$tab}{ \"%02d\", ${max} },\n", $ai);
            }
        }
    } else {
        if ($ais >= $from && $ais < $limit && !$used) {
            if ($ais >= $prev_unit) {
                print "\n";
                $prev_unit = ($ais * $unit) / $unit + $unit;
            }
            printf("{$tab}{ \"%02d\", ${max} },\n", $ais);
        }
    }
}

// Print output

if ($tag === '') {
    $tag = 'WARNING: tag not set!';
}
print <<<EOD
// https://github.com/gs1/gs1-syntax-dictionary $tag
static const AiInfo aiInfos[] = {
//TWO_DIGIT_DATA_LENGTH
EOD;
$prev_unit = 0;
foreach ($ais_maxs as list($ais, $max)) {
    print_ai($tab, $ais, $max, 0, 100, $prev_unit, 10);
}

print <<<EOD

//THREE_DIGIT_DATA_LENGTH
EOD;
$prev_unit = 0;
foreach ($ais_maxs as list($ais, $max)) {
    print_ai($tab, $ais, $max, 100, 1000, $prev_unit, 100);
}

// AIs "3[1234569]nn" and "703n" and "723n" need special treatment to match `AiInfo::aiSize()` logic
print <<<EOD

//THREE_DIGIT_PLUS_DIGIT_DATA_LENGTH

EOD;
foreach ($ais_maxs as list($ais, $max, &$used)) {
    if (is_array($ais)) {
        if (($ais[0] >= 3100 && $ais[0] < 3700) || ($ais[0] >= 3900 && $ais[0] < 4000)) {
            printf("{$tab}{ \"%d\", ${max} },\n", $ais[0] / 10);
            $used = true;
        }
    } else {
        if (($ais >= 7030 && $ais < 7040) || ($ais >= 7230 && $ais < 7240)) {
            if ($ais == 7030 || $ais == 7230) {
                printf("{$tab}{ \"%d\", ${max} },\n", $ais / 10);
            }
            $used = true;
        }
    }
}

print <<<EOD

//FOUR_DIGIT_DATA_LENGTH
EOD;
$prev_unit = 0;
foreach ($ais_maxs as list($ais, $max, $used)) {
    print_ai($tab, $ais, $max, 1000, 10000, $prev_unit, 1000, $used);
}

print <<<EOD
};

EOD;

/* vim: set ts=4 sw=4 et : */
