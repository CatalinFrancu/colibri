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

    <div class="menu">
      <ul>
        <li><a href="../www/">home</a></li>
        <li><a href="about.php">about</a></li>
      </ul>
    </div>

    {include file=$template}

    <div id="footer">
      Copyright 2013 <a href="http://catalin.francu.com">Catalin Francu</a><br/>

      Colibri's source code is released under the <a href="http://www.gnu.org/licenses/agpl-3.0.html">GNU Affero General Public License</a>.<br/>

      Colibri's data is released under the
      <a href="http://creativecommons.org/licenses/by-sa/3.0/">Creative Commons Attribution-ShareAlike 3.0 Unported License</a>.
    </div>

    {* Google Analytics code *}
    {literal}
    <script type="text/javascript">
      var _gaq = _gaq || [];
      _gaq.push(['_setAccount', 'UA-37655508-1']);
      _gaq.push(['_trackPageview']);

      (function() {
      var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
      ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
      var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
      })();
    </script>
    {/literal}
  </body>
</html>
