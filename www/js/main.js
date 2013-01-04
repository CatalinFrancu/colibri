currentHammer = null;

$(function() {
  $('#editLink').click(editLinkClick);
  $('.hammer').click(hammerClick);
  $('.anvil').click(anvilClick);
  $('#stm').click(stmClick);
  $('#goButton').click(goButtonClick);
});

function editLinkClick() {
  $('#toolBox').slideToggle('slow');
  return false;
}

function hammerClick() {
  if (currentHammer) {
    currentHammer.removeClass('currentHammer');
  }
  $(this).addClass('currentHammer');
  currentHammer = $(this);
}

function anvilClick() {
  if (!currentHammer) {
    return;
  }
  var hammerClass = currentHammer.children('div').attr('class');
  if (!hammerClass) {
    hammerClass = '';
  }
  $(this).children('div').attr('class', hammerClass);
}

function stmClick() {
  $(this).removeClass(currentStm + 'k');
  currentStm = (currentStm == 'w') ? 'b' : 'w';
  $(this).addClass(currentStm + 'k');
}

function goButtonClick() {
  var fen = '';
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
    } else {
      if (numEmpty) {
        fen += numEmpty;
        numEmpty = 0;
      }
      // First character is w/b, second character is the piece name
      fen += (cl[0] == 'w') ? cl[1].toUpperCase() : cl[1];
    }
  });
  if (numEmpty) {
    fen += numEmpty;
  }
  fen += ' ' + currentStm + ' - - 0 0';
  $('#fen').val(fen);
  $('#editForm').submit();
}
