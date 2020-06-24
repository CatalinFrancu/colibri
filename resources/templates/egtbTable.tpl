<h3 class="parentScore">
  EGTB: {include "egtbScore.tpl" score=$response.score}
</h3>

<table class="table table-sm">
  <thead>
    <tr>
      <th scope="col">#</th>
      <th scope="col">move</th>
      <th scope="col">score</th>
    </tr>
  </thead>
  <tbody>
    {foreach $response.children as $i => $c}
      <tr>
        <td>
          {$i + 1}
        </td>
        <td>
          <a href="?fen={$c.fen|escape:url}">{$c.move}</a>
        </td>
        <td>
          {include "egtbScore.tpl" score=-$c.score}
        </td>
      </tr>
    {/foreach}
  </tbody>
</table>
