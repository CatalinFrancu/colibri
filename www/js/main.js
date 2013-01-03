currentHammer = null;

$(function() {
  $('.hammer').click(hammerClick);
  $('.anvil').click(anvilClick);
});

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
