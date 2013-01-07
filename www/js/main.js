currentHammer = null;

$(function() {
  $('#editLink').click(editLinkClick);
  $('.hammer').click(hammerClick);
  $('.eraseAll').click(eraseAllClick);
  $('.anvil').click(anvilClick);
  $('.stm').click(stmClick);
  $('#goButton').click(goButtonClick);
});

function editLinkClick() {
  $('#toolBox').slideToggle('fast');
  return false;
}

function hammerClick() {
  if (currentHammer) {
    currentHammer.removeClass('currentHammer');
  }
  $(this).addClass('currentHammer');
  currentHammer = $(this);
}

function eraseAllClick() {
  $('.anvil').children('div').attr('class', '');
}

function anvilClick() {
  if (!currentHammer) {
    return;
  }
  var hammerClass = currentHammer.children('div').attr('class');
  if (hammerClass == 'eraseOne') {
    $(this).children('div').attr('class', '');
  } else {
    $(this).children('div').attr('class', hammerClass);
  }
}

function stmClick() {
  $('.stm').removeClass('stmSelected');
  $(this).addClass('stmSelected');
  currentStm = ($(this).html() == 'white') ? 'w' : 'b';
}

function goButtonClick() {
  var fen = '';
  var epSquare = '';
  var numPieces = 0;
  var numEmpty = 0;
  $('#board').find('div').each(function(i) {
    // End previous rank: add empty squares, add a slash
    if (i && (i % 8 == 0)) {
      if (numEmpty) {
        fen += numEmpty;
        numEmpty = 0;
      }
      fen += '/';
    }
    var cl = $(this).attr('class');
    if (!cl) {
      numEmpty++;
    } else if (cl == 'epSquare') {
      epSquare += $(this).attr('id');
    } else {
      if (numEmpty) {
        fen += numEmpty;
        numEmpty = 0;
      }
      // First character is w/b, second character is the piece name
      fen += (cl[0] == 'w') ? cl[1].toUpperCase() : cl[1];
      numPieces++;
    }
  });
  if (numEmpty) {
    fen += numEmpty;
  }

  var errorMsg = '';
  if (!numPieces) {
    errorMsg = 'Please place some pieces on the board.';
  } else if (epSquare.length > 2) {
    errorMsg = 'There can be at most one en passant square.';
  }

  if (!errorMsg && epSquare) {
    var epRank = parseInt(epSquare[1]);
    var epFile = epSquare[0];
    var pawnRank = (currentStm == 'w') ? (epRank - 1) : (epRank + 1);
    var emptyRank = (currentStm == 'w') ? (epRank + 1) : (epRank - 1);
    var expectedPawn = (currentStm == 'w') ? 'bp' : 'wp';
    var expectedColor = (currentStm == 'w') ? 'Black' : 'White';
    if ((currentStm == 'w') && (epRank != 6)) {
      errorMsg = 'When White is to move, the en passant square (if any) must be on the 6th rank.';
    } else if ((currentStm == 'b') && (epRank != 3)) {
      errorMsg = 'When Black is to move, the en passant square (if any) must be on the 3rd rank.';
    } else if (expectedPawn != $('#' + epFile + pawnRank).attr('class')) {
      errorMsg = 'Expecting a ' + expectedColor + ' pawn at ' + epFile + pawnRank;
    } else if ($('#' + epFile + emptyRank).attr('class')) {
      errorMsg = 'Expecting an empty square at ' + epFile + emptyRank;
    }
  }

  // Make sure all the pawns are on ranks 2-7
  $('#board .wp, #board .bp').each(function() {
    var square = $(this).attr('id');
    var rank = parseInt(square[1]);
    if (rank < 2 || rank > 7) {
      errorMsg = 'Pawns can only sit on ranks 2-7.';
    }
  });

  if (errorMsg) {
    alert(errorMsg);
  } else {
    fen += ' ' + currentStm + ' - ' + (epSquare ? epSquare : '-') + ' 0 0';
    $('#fenField').val(fen);
    $('#editForm').submit();
  }
}
