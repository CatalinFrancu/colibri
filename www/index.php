<?php

require_once __DIR__ . '/../resources/Core.php';

$config = parse_ini_file(__DIR__ . '/../colibri.conf');

$egtb = isset($_GET['egtb']); // request a random EGTB board
$fen = $_GET['fen'] ?? 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1';

if ($egtb) {
  list($board, $stm) = randomBoard(5);
  $fen = boardToFen($board, $stm);
} else {
  $stm = explode(' ', $fen)[1];
}

$smarty = Core::getSmarty();
try {
  $response = serverQuery($config, $fen);
  $smarty->assign('response', $response);
} catch (Exception $e) {
  $smarty->assign('error', $e->getMessage());
}

$smarty->assign([
  'fen' => $fen,
  'stm' => $stm,
  'template' => 'index.tpl',
]);
$smarty->display('layout.tpl');

/***************************************************************************/

function randomBoard($numPieces) {
  // No queens -- they generate boring convert-in-2 positions.
  $pieceNames = [ 'p', 'n', 'b', 'r', 'k' ];
  $board = [];
  for ($i = 8; $i >= 1; $i--) {
    for ($j = 'a'; $j <= 'h'; $j++) {
      $board[$i][$j]['piece'] = '';
    }
  }
  for ($k = 0; $k < $numPieces; $k++) {
    do {
      $i = rand(1, 8);
      $j = chr(ord('a') + rand(0, 7));
    } while ($board[$i][$j]['piece']);
    $color = ($k < 2) ? $k : rand(0, 1); // First two pieces to be placed are of opposite colors
    $minPiece = ($i == 1 || $i == 8) ? 1 : 0; // No pawns on first/last rank
    $piece =  $pieceNames[rand($minPiece, count($pieceNames) - 1)];
    $board[$i][$j]['piece'] = ($color ? 'w' : 'b') . $piece;
  }
  $stm = (rand(0, 1) == 1) ? 'w' : 'b';
  return [ $board, $stm ];
}

function boardToFen($board, $stm) {
  $fen = '';
  for ($rank = 8; $rank >= 1; $rank--) {
    if ($rank < 8) {
      $fen .= '/';
    }
    $numEmpty = 0;
    for ($file = 'a'; $file <= 'h'; $file++) {
      $str = $board[$rank][$file]['piece'];
      if (!$str) {
        $numEmpty++;
      } else {
        if ($numEmpty) {
          $fen .= $numEmpty;
          $numEmpty = 0;
        }
        $fen .= ($str[0] == 'w') ? strtoupper($str[1]) : $str[1];
      }
    }
    if ($numEmpty) {
      $fen .= $numEmpty;
    }
  }
  $fen .= " $stm - - 0 0";
  return $fen;
}

// Sort the children. Note that child scores are reversed from the parent's
// point of view.
function egtbCmp($a, $b) {
  $as = $a['score'];
  $bs = $b['score'];
  if ($as == $bs) {
    return strcasecmp($a['move'], $b['move']);
  }

  if ((($as > 0) && ($bs > 0)) ||     // both winning? prefer the faster win
      ((($as < 0) && ($bs < 0))))  {  // both losing? prefer the slower loss
    return ($bs - $as);
  }
  return $as - $bs;
}

function readEgtbResponse(&$sock) {
  $r = [
    'type' => 'egtb',
    'score' => fscanf($sock, '%d ')[0],
    'numChildren' => fscanf($sock, '%d ')[0],
    'children' => [],
  ];
  for ($i = 0; $i < $r['numChildren']; $i++) {
    $line = trim(fgets($sock));
    $parts = explode(' ', $line, 3);
    $r['children'][] = [
      'move' => $parts[0],
      'score' => $parts[1],
      'fen' => $parts[2],
    ];
  }

  usort($r['children'], 'egtbCmp');
  return $r;
}

function readPnsResponse(&$sock) {
  $r = [
    'type' => 'pns',
    'proof' => fscanf($sock, '%d')[0],
    'disproof' => fscanf($sock, "%d")[0],
    'numChildren' => fscanf($sock, "%d")[0],
    'children' => [],
  ];
  for ($i = 0; $i < $r['numChildren']; $i++) {
    $line = trim(fgets($sock));
    $parts = explode(' ', $line, 4);
    $r['children'][] = [
      'move' => $parts[0],
      'proof' => $parts[1],
      'disproof' => $parts[2],
      'fen' => $parts[3],
    ];
  }

  return $r;
}

function serverQuery($config, $fen) {
  $sock = @fsockopen('localhost', $config['queryServerPort']);
  if (!$sock) {
    throw new Exception('Backend server not responding.');
  }

  fprintf($sock, "query $fen\n");
  $type = trim(fgets($sock));

  switch ($type) {
    case 'error':
      throw new Exception(trim(fgets($sock)));

    case 'egtb':
      return readEgtbResponse($sock);

    case 'pns':
      return readPnsResponse($sock);

    case 'unknown':
      return [ 'type' => 'unknown' ];
  }
}
