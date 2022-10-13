<?php
/*
* Copyright 2022 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

// Generate tables and if conditions for `zx_iswgraph()` ("core/src/ZXCType.cpp")
//
// To run (from "core/tools" directory):
//     php gen_zx_iswgraph.php
//
// `zend.assertions` should set to 1 in "php.ini"

$basename = basename(__FILE__);

// Using Unicode 15.0.0 (draft as of 2022-04-26)
$unicode_ver = '15.0.0';
$file = 'https://www.unicode.org/Public/15.0.0/ucd/UnicodeData.txt';

if (($get = file_get_contents($file)) === false) {
	exit("$basename:" . __LINE__ . " ERROR: Could not read file \"$file\"" . PHP_EOL);
}

$graphs = array();
for ($i = 0; $i <= 0x10; $i++) {
	$graphs[$i] = array();
}
$lines = explode("\n", $get);
$line_no = 0;
$have_first = false;
$z_cnt = 0;
foreach ($lines as $line) {
	$line_no++;
	if ($line === '') {
		continue;
	}
	if (!preg_match('/^([0-9A-F]+);([^;]*);([CLMNPSZ])/', $line, $matches)) {
		exit("$basename:" . __LINE__ . " ERROR: Could not parse line $line_no" . PHP_EOL);
	}
	$u = hexdec($matches[1]);
	$name = $matches[2];
	$category = $matches[3];

	// Deal with First/Last codepoints
	if ($have_first) {
		if (strpos($name, ', Last>') === false) {
			exit("$basename:" . __LINE__ . " ERROR: First with no following Last line $line_no" . PHP_EOL);
		}
		if ($category !== $first_category) {
			exit("$basename:" . __LINE__ . " ERROR: Last category different than First line $line_no" . PHP_EOL);
		}
		$have_first = false;
	} else if (strpos($name, ', First>') !== false) {
		$have_first = true;
		$first_u = $u;
		$first_category = $category;
		continue;
	} else {
		$first_u = $u;
	}

	if ($category === 'C' || $category === 'Z') {
		if ($category === 'Z') {
			//printf("%d: U+%04X\n", ++$z_cnt, $u);
		}
		// Visible exceptions to C
		if ($u >= 0x600 && $u <= 0x605) { // ARABIC NUMBER SIGN..ARABIC NUMBER MARK ABOVE
		} else if ($u === 0x08E2) { // ARABIC DISPUTED END OF AYAH
		} else if ($u === 0x110BD || $u === 0x110CD) { // KAITHI NUMBER SIGN, KAITHI NUMBER SIGN ABOVE
			// Fall-thru
		} else {
			continue;
		}
	}
	for ($i = $first_u; $i <= $u; $i++) {
		if ($i > 0x10FFFF) {
			exit("$basename:" . __LINE__ . " ERROR: Invalid Unicode $i at line $line_no" . PHP_EOL);
		}
		$graphs[$i >> 16][$i - ($i & 0x1F0000)] = true;
	}
}

/* Output tables to `$out` array */
function out_tab(&$out, $name, $graph, $skip_func, $comment = '', $unassigned_func = null) {

	if ($comment) {
		$out[] = "\t" . '/* ' . $comment . ' */';
	}
	$last = array_keys($graph)[count($graph) - 1];

	$def_i = count($out);
	$out[] = ''; /* Placeholder for definition */
	$begin_line = "\t\t";
	$line = $begin_line;
	$entry = 0;
	$in_skip = false;
	$j = 0;
	for ($i = 0; $i <= $last; $i++) {
		if ($skip_func($i)) {
			if ($unassigned_func && $unassigned_func($i)) {
				assert(isset($graph[$i]) === false);
			}
			if (!$in_skip) {
				// Assuming blocks of 8
				$line .= sprintf('%s0x%02X,', $line === $begin_line ? '' : ' ', $entry);
				$j++;
				if ($j % 8 === 0) {
					$out[] = $line . sprintf(" /*%04X*/", $i);
					$line = $begin_line;
				}
				$entry = 0;
				$in_skip = true;
			}
			continue;
		}
		if ($i && $i % 8 === 0 && !$in_skip) {
			$line .= sprintf('%s0x%02X,', $line === $begin_line ? '' : ' ', $entry);
			$j++;
			if ($j % 8 === 0) {
				$out[] = $line . sprintf(" /*%04X*/", $i);
				$line = $begin_line;
			}
			$entry = 0;
		}
		$in_skip = false;
		if (isset($graph[$i])) {
			$entry |= 1 << ($i % 8);
		}
	}
	$line .= sprintf('%s0x%02X,', $line === $begin_line ? '' : ' ', $entry);
	$j++;
	$out[] = $line . sprintf(" /*%04X*/", $i);
	$out[] = "\t" . '};';
	$out[$def_i] = "\t" . 'static const unsigned char zx_graph_' . $name . '[' . $j . '] = {';
}

/* Output if ranges to `$out` array */
function out_ranges(&$out, $graph, $start) {
	$cnt = count($graph);
	$j = 0;
	$prev_start = $prev = -1;
	$ranges = array();
	foreach ($graph as $i => $dummy) {
		if ($prev === -1) {
			$prev_start = $prev = $i;
		} else {
			if ($i === $prev + 1) {
				$prev = $i;
			} else {
				$ranges[] = array($prev_start, $prev);
				$prev_start = $prev = $i;
			}
		}
	}
	$ranges[] = array($prev_start, $prev);
	$tot = 0;
	$out[] = sprintf("\tif (u <= 0x%X) {", $start + 0xFFFF);
	$line = "\t\t";
	$begin_con_line = "\t\t\t\t";
	foreach ($ranges as $i => $range) {
		$tot += $range[1] - $range[0] + 1;
		if (strlen($line) > 100) {
			$out[] = $line;
			$line = $begin_con_line;
		}
		if ($range[0] === $range[1]) {
			if ($i === 0) {
				$line .= sprintf('return u == 0x%X', $start + $range[0]);
			} else {
				$line .= sprintf('%s|| u == 0x%X', $line === $begin_con_line ? '' : ' ', $start + $range[0]);
			}
		} else {
			if ($i === 0) {
				$line .= sprintf('return (u >= 0x%X && u <= 0x%X)', $start + $range[0], $start + $range[1]);
			} else {
				$line .= sprintf('%s|| (u >= 0x%X && u <= 0x%X)',
							$line === $begin_con_line ? '' : ' ', $start + $range[0], $start + $range[1]);
			}
		}
	}
	$out[] = $line . ';';
	$out[] = "\t}";
	assert($tot === $cnt);
}

$out = array();

if (0) {
	print "Count ";
	for ($i = 0; $i <= 0x10; $i++) {
		printf(" %X: %d", $i, count($graphs[$i]));
	}
	print PHP_EOL;
}

$out[] = "\t" . '// Begin copy/paste of `zx_iswgraph()` tables output from "core/tools/gen_zx_iswgraph.php"';

assert(count($graphs[0]));
out_tab($out, 'bmp', $graphs[0],
	function($u) {
		return ($u >= 0x3400 && $u <= 0x4DBF) // CJK Ideograph Extension A (all graphical)
			|| ($u >= 0x4E00 && $u <= 0x9FFF) // CJK Ideograph (URO) (all graphical)
			|| ($u >= 0xAC00 && $u <= 0xD79F) // Hangul Syllable (URO) (all graphical) (Note 0xD7A0-D7A3 not skipped)
			|| ($u >= 0xD800 && $u <= 0xDFFF) // Surrogates (all non-graphical)
			|| ($u >= 0xE000 && $u <= 0xF8FF); // PUA (treat all as non-graphical)
	}, "Unicode $unicode_ver"
);

assert(count($graphs[1]));
out_tab($out, '1', $graphs[1],
	function($u) {
		return ($u >= 0x25C0 && $u <= 0x2F7F) // Unassigned pages 0x125C0-12F7F
			|| ($u >= 0x3480 && $u <= 0x43FF) // Unassigned pages 0x13480-14400
			|| ($u >= 0x4700 && $u <= 0x67FF) // Unassigned pages 0x14700-167FF
			|| ($u >= 0x7000 && $u <= 0x87F7) // Tangut Ideograph
			|| ($u >= 0x8D00 && $u <= 0x8D07) // Tangut Ideograph Supplement (Note 0x18D08 not skipped)
			|| ($u >= 0x8E00 && $u <= 0xAEFF) // Unassigned pages 0x18E00-1AEFF
			|| ($u >= 0xB300 && $u <= 0xBBFF) // Unassigned pages 0x1B300-1BBFF
			|| ($u >= 0xBD00 && $u <= 0xCEFF); // Unassigned pages 0x1BD00-1CEFF
	}, null, function($u) { /* For checking actually unassigned */
		return ($u >= 0x25C0 && $u <= 0x2F7F) // Unassigned pages 0x125C0-12F7F
			|| ($u >= 0x3480 && $u <= 0x43FF) // Unassigned pages 0x13480-14400
			|| ($u >= 0x4700 && $u <= 0x67FF) // Unassigned pages 0x14700-167FF
			|| ($u >= 0x8E00 && $u <= 0xAEFF) // Unassigned pages 0x18E00-1AEFF
			|| ($u >= 0xB300 && $u <= 0xBBFF) // Unassigned pages 0x1B300-1BBFF
			|| ($u >= 0xBD00 && $u <= 0xCEFF); // Unassigned pages 0x1BD00-1CEFF
	}
);

$out[] = "\t" . '// End copy/paste of `zx_iswgraph()` tables output from "core/tools/gen_zx_iswgraph.php"';

assert(count($graphs[0x2])); // Dealt with by out_ranges() below
assert(count($graphs[0x3])); // Dealt with by out_ranges() below
for ($i = 4; $i <= 0xD; $i++) {
	assert(count($graphs[$i]) === 0);
}
assert(count($graphs[0xE])); // Dealt with by out_ranges() below
assert(count($graphs[0xF]) === 0);
assert(count($graphs[0x10]) === 0);

$out[] = "\t" . '// Begin copy/paste of `zx_iswgraph()` if conditions output from "core/tools/gen_zx_iswgraph.php"';

out_ranges($out, $graphs[0x2], 0x20000);
out_ranges($out, $graphs[0x3], 0x30000);
out_ranges($out, $graphs[0xE], 0xE0000);

$out[] = "\t" . '// End copy/paste of `zx_iswgraph()` if conditions output from "core/tools/gen_zx_iswgraph.php"';

print implode("\n", $out) . "\n";
