<!DOCTYPE html>
<html>
  <head>
    <title>Colibri Suicide Chess Browser</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
    <link href="css/main.css?v=1" rel="stylesheet" type="text/css"/>
    <script src="js/jquery-1.8.3.min.js"></script>
    <script src="js/main.js?v=1"></script>
  </head>

  <body>
    <div class="title">
      Suicide chess browser by <span class="plug">Colibri</span>
    </div>

    <div id="leftColumn">
      <div id="fen">FEN: {$fen}</div>
      <table id="board">
        <tr>
          <th class="lr tb"></th>
          {foreach from=$board[8] item=square key=fileName}
          <th class="tb">{$fileName}</th>
          {/foreach}
          <th></th>
        </tr>
        {foreach from=$board item=rank key=rankNumber}
          <tr>
            <th class="lr">{$rankNumber}</th>
            {foreach from=$rank item=square key=fileLetter}
              <td class="square anvil {$square.color}"><div id="{$fileLetter}{$rankNumber}" class="{$square.piece}"></div></td>
            {/foreach}
            <th class="lr">{$rankNumber}</th>
          </tr>
        {/foreach}
        <tr>
          <th class="lr tb"></th>
          {foreach from=$board[8] item=square key=fileName}
          <th class="tb">{$fileName}</th>
          {/foreach}
          <th></th>
        </tr>
      </table>

      <div>
        <div id="editLinkDiv"><a id="editLink" href="#">edit position</a></div>
        <div id="stmDiv" class="{if $stm == 'w'}stmReadWhite{else}stmReadBlack{/if}">to move</div>
        <div style="clear: both"></div>
      </div>

      <div id="toolBox">
        Click on a piece, then click on a board square to place it.
        <table id="editBox">
          <tr>
            <td class="square hammer"><div class="wk"></div></td>
            <td class="square hammer"><div class="wq"></div></td>
            <td class="square hammer"><div class="wr"></div></td>
            <td class="square hammer"><div class="wb"></div></td>
            <td class="square hammer"><div class="wn"></div></td>
            <td class="square hammer"><div class="wp"></div></td>
          </tr>
          <tr>
            <td class="square hammer"><div class="bk"></div></td>
            <td class="square hammer"><div class="bq"></div></td>
            <td class="square hammer"><div class="br"></div></td>
            <td class="square hammer"><div class="bb"></div></td>
            <td class="square hammer"><div class="bn"></div></td>
            <td class="square hammer"><div class="bp"></div></td>
          </tr>
          <tr>
            <td class="square hammer" title="En passant target square"><div class="epSquare"></div></td>
            <td class="square hammer" title="Erase one piece"><div class="eraseOne"></div></td>
            <td class="square eraseAll" title="Erase the entire board"><div></div></td>
          </tr>
        </table>

        Side to move:
        <div id="stmBox">
          <div class="stm stmWhite {if $stm == 'w'}stmSelected{/if}">white</div>
          <div class="stm stmBlack {if $stm == 'b'}stmSelected{/if}">black</div>
          <form id="editForm" action="index.php">
            <input id="fenField" type="hidden" name="fen" value=""/>
            <input id="goButton" type="button" value="Go!"/>
          </form>
          <div style="clear: both"></div>
        </div>
      </div>
    </div>

    <div id="rightColumn">
      Score: {$score}
      <table id="moves">
        {foreach from=$moves item=m}
          <tr>
            <td>
              <a href="?fen={$m.fen|escape}">{$m.move}</a>
            </td>
            <td class="{include file=scoreClass.tpl score=$m.score}">{include file=score.tpl score=$m.score}</td>
          </tr>
        {/foreach}
      </table>
    </div>

    <script>
      currentStm = '{$stm}';
    </script>
  </body>
</html>
