{if isset($error)}
  <div class="alert alert-danger">
    {$error}
  </div>
{else}
  <div class="row">
    <div class="col-lg">
      <div id="board"></div>

      <form class="mt-3">
        <div class="form-group row">
          <div class="col-md-6">
            <button id="startBtn" type="button" class="btn btn-sm btn-secondary">
              start position
            </button>
            <button id="clearBtn" type="button" class="btn btn-sm btn-secondary">
              clear board
            </button>
          </div>

          <div class="col-md-6">
            <select id="stm" class="form-control form-control-sm">
              <option value="w"">white to move</option>
              <option value="b" {if $stm == 'b'}selected{/if}>black to move</option>
            </select>
          </div>
        </div>

        <div class="form-group row">
          <small class="col form-text text-muted">
            Click on an empty square on the third or sixth rank to set the en
            passant target square.
          </small>
        </div>

        <div class="d-flex">
          <div class="mr-1">
            <label for="fen" class="col-form-label form-control-sm pl-0">FEN:</label>
          </div>
          <div class="flex-grow-1">
            <input
              id="fen"
              type="text"
              class="form-control form-control-sm"
              name="fen"
              value="{$fen}"/>
          </div>
          <div class="ml-1">
            <button
              type="submit"
              id="goButton"
              class="btn btn-sm btn-primary">go</button>
          </div>
        </div>
      </form>
    </div>

    <div class="col-lg">
      {if $response.type == 'egtb'}
        {include "egtbTable.tpl"}
      {elseif $response.type == 'pns'}
        {include "pnsTable.tpl"}
      {elseif $response.type == 'unknown'}
        {include "unknownTable.tpl"}
      {/if}
    </div>
  </div>
{/if}
