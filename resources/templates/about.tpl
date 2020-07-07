<h3>About Colibri</h3>

<p>
  Colibri is a <a href="http://en.wikipedia.org/wiki/Antichess">suicide chess</a> engine. It
  plays according to
  <a href="https://en.wikipedia.org/wiki/Losing_chess#Variations_regarding_stalemate">joint rules</a>
  for stalemate.
</p>

<p>
  Suicide chess was weakly solved. Specifically, White wins after <b>1.</b> e3. Some
  lines, such as <b>1.</b> e3 d5 or <b>1.</b> e3 e5, are trivially solvable by
  average players. Other lines, like <b>1.</b> e3 b5 took years of computing
  time from several different chess programs. The bulk of the work was done my
  <a href="http://magma.maths.usyd.edu.au/~watkins/LOSING_CHESS/">Mark Watkins</a>.
</p>

<p>
  My aim in writing Colibri (aside from having fun and polishing my
  programming skills) is to tackle some defenses for Black and find wins for
  White. I may end up connecting this program to lichess so it can play humans
  or other computers, but that is not my short-term goal. For example, I
  probably won't implement an alpha-beta algorithm.
</p>

<h3>Features</h3>

<ul>
  <li>5-men <a href="http://en.wikipedia.org/wiki/Endgame_tablebase">endgame tablebases</a> (EGTB) with en passant support (52 GB);</li>

  <li>EGTB compression using <a href="http://tukaani.org/xz/">liblzma</a> (xz format, 32 KB block size, which gives an 11% compression rate);</li>

  <li>EGTB are computed using retrograde analysis and stored as DTC (distance-to-conversion). Scores are truncated at +/- 127 so they fit on one
  byte. The only tables where this happens are KBNvK, KRPvKP and KBNPvK. Hopefully this won't cause Colibri to miss any important wins.</li>

  <li>Board representation using <a href="http://en.wikipedia.org/wiki/Bitboard">bitboards</a>.</li>
</ul>

<h3>TODO list</h3>

<ul>
  <li>Fix some bugs in <a href="http://en.wikipedia.org/wiki/Proof-number_search">proof number search</a> so I can start solving openings.</li>
</ul>

<h3>Nilatac</h3>

<p>My previous suicide chess engine was <a href="http://catalin.francu.com/nilatac/book.php">Nilatac</a>, written mostly between 2001-2003. I decided
to give it up and rewrite Colibri from scratch because Nilatac had several shortcomings:</p>

<ul>
  <li>Slow move generator;</li>
  <li>Crashes and infinite loops in the proof-number search algorithm, since it ignores transpositions and repetitions; </li>
  <li>EGTB bugs, at least due to the lack of en passant support, and possibly more since the EGTB were never verified;</li>
  <li>slow EGTB generation -- it would take forever to generate the 5-men EGTB, because it didn't use retrograde analysis;</li>
  <li>no EGTB compression;</li>
  <li>no unit tests.</li>
</ul>

<h3>Downloads</h3>

<p>Colibri is free software. You can <a href="https://github.com/CatalinFrancu/colibri/">browse the source code</a>.

<p>I will seed the EGTB files on torrents once anybody asks. :-)</p>

<h3>License</h3>

<p>Copyright 2013-2020 <a href="http://catalin.francu.com">Catalin Francu</a></p>

<p>Colibri's source code is released under the <a href="http://www.gnu.org/licenses/agpl-3.0.html">GNU Affero General Public License</a>.</p>

<p>Colibri's endgame tables are released under the <a href="http://creativecommons.org/licenses/by-sa/3.0/">Creative Commons Attribution-ShareAlike
3.0 Unported License</a>.</p>
