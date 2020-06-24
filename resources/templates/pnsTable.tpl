<h3 class="parentScore">
  Book:
  {include "proofNumber.tpl" n=$response.proof}
  /
  {include "proofNumber.tpl" n=$response.disproof}
</h3>

<table class="table table-sm">
  <thead>
    <tr>
      <th scope="col">#</th>
      <th scope="col">move</th>
      <th scope="col">proof</th>
      <th scope="col">disproof</th>
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
          {include "proofNumber.tpl" n=$c.proof}
        </td>
        <td>
          {include "proofNumber.tpl" n=$c.disproof}
        </td>
      </tr>
    {/foreach}
  </tbody>
</table>
