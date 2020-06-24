$(function() {

  var fen; // board only (first field of the FEN notation)
  var stm;
  var epSquare = '-';

  init();

  function init() {
    if (!$('#board').length) {
      return;
    }

    var config = {
      draggable: true,
      dropOffBoard: 'trash',
      sparePieces: true,
      onChange: boardChange,
      pieceTheme: pieceTheme,
      position: $('#fen').val(),
    }
    var board = Chessboard('board', config);

    fen = board.fen();
    stm = $('#stm').val();
    epSquare = $('#fen').val().split(' ')[3];

    if (epSquare != '-') {
      $(`#board .square-${epSquare}`).addClass('en-passant');
    }

    $('#startBtn').click(board.start);
    $('#clearBtn').click(board.clear);
    $('#stm').change(stmChange);
    $('#board div[data-square]').click(squareClick);
    $('#goButton').click(goButtonClick);
  }

  function pieceTheme(piece) {
    return 'img/pieces/' + piece + '.svg';
  }

  function boardChange(oldPos, newPos) {
    fen = Chessboard.objToFen(newPos);
    updateFenField();
  }

  function updateFenField() {
    $('#fen').val(`${fen} ${stm} - ${epSquare} 0 1`);
  }

  function stmChange() {
    stm = $('#stm').val();
    updateFenField;
  }

  function squareClick() {
    if ($(this).hasClass('en-passant')) {
      $(this).removeClass('en-passant');
      epSquare = '-';
      updateFenField();
      return;
    }

    var square = $(this).data('square');
    var file = square[0];
    var rank = Number(square[1]);

    // Skip the empty square check. It should be empty because the draggable
    // property eats all clicks on occupied squares.

    // Check that there is a pawn in front and an empty square behind.
    if (((rank == 3) &&
         ($('#board .square-' + file + '4 img[data-piece="wP"]').length == 1) &&
         ($('#board .square-' + file + '2 img').length == 0) &&
         (stm == 'b')) ||
        ((rank == 6) &&
         ($('#board .square-' + file + '5 img[data-piece="bP"]').length == 1) &&
         ($('#board .square-' + file + '7 img').length == 0) &&
         (stm == 'w'))) {
      epSquare = square;
      $('.en-passant').removeClass('en-passant');
      $(`#board .square-${square}`).addClass('en-passant');
      updateFenField();
    }
  }

  function goButtonClick() {
    // check that there are some pieces
    var numPieces = $('#board div[data-square] img[data-piece]').length;
    if (!numPieces) {
      alert('Please place some pieces on the board.');
      return false;
    }

    // check that there are no pawns on rows 1 and 8
    var numPawnsFirst = $('#board div[data-square$="1"] img[data-piece$="P"]').length;
    var numPawnsEighth = $('#board div[data-square$="8"] img[data-piece$="P"]').length;
    if (numPawnsFirst || numPawnsEighth) {
      alert('Pawns are not allowed on the first or eighth rank.');
      return false;
    }

    return true;
  }
});
