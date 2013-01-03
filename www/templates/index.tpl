<!DOCTYPE html>
<html>
  <head>
    <title>Title of the document</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
    <link href="css/main.css?v=1" rel="stylesheet" type="text/css"/>
    <script src="js/jquery-1.8.3.min.js"></script>
    <script src="js/main.js?v=1"></script>
  </head>

  <body>
    <table id="board">
      {foreach from=$board item=rank}
        <tr>
          {foreach from=$rank item=square}
            <td class="square anvil {$square.color}"><div class="{$square.piece}"></div></td>
          {/foreach}
        </tr>
      {/foreach}
    </table>

    <table id="editBox">
      <tr>
        <td class="square hammer"><div class="wk"></div></td>
        <td class="square hammer"><div class="wq"></div></td>
        <td class="square hammer"><div class="wr"></div></td>
        <td class="square hammer"><div class="wb"></div></td>
        <td class="square hammer"><div class="wn"></div></td>
        <td class="square hammer"><div class="wp"></div></td>
        <td rowspan="2" class="square hammer"><div>(empty)</div></td>
      </tr>
      <tr>
        <td class="square hammer"><div class="bk"></div></td>
        <td class="square hammer"><div class="bq"></div></td>
        <td class="square hammer"><div class="br"></div></td>
        <td class="square hammer"><div class="bb"></div></td>
        <td class="square hammer"><div class="bn"></div></td>
        <td class="square hammer"><div class="bp"></div></td>
      </tr>
    </table>
  </body>
</html>
