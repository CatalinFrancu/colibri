<!doctype html>
<html lang="en">
  <head>
    <title>Colibri Suicide Chess Browser</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
    <link href="css/third-party/bootstrap-4.3.1.min.css" rel="stylesheet" type="text/css">
    <link href="css/main.css?v=1" rel="stylesheet" type="text/css"/>
    <script src="js/third-party/jquery-3.5.1.min.js"></script>
    <script src="js/main.js?v=1"></script>
  </head>

  <body>
    <nav class="navbar navbar-expand-md navbar-dark bg-dark">
      <span class="navbar-brand">
        Antichess browser by <span class="plug">Colibri</span>
      </span>

      <button
        class="navbar-toggler"
        type="button"
        data-toggle="collapse"
        data-target="#navbarMenu">
        <span class="navbar-toggler-icon"></span>
      </button>

      <div class="collapse navbar-collapse" id="navbarMenu">
        <ul class="navbar-nav ml-3">
          <li class="nav-item">
            <a class="nav-link" href="../www/">home</a>
          </li>
          <li class="nav-item">
            <a class="nav-link" href="about.php">about</a>
          </li>
        </ul>
      </div>
    </nav>

    <main class="container-fluid">
      <div class="container mt-4">
        {include file=$template}
      </div>
    </main>

    <footer class="py-3 border-top text-center">
      Copyright 2013-2020 <a href="http://catalin.francu.com">Catalin Francu</a>

      <span class="text-muted px-3">•</span>

      <a href="https://github.com/CatalinFrancu/colibri/">source code</a>
    </footer>

  </body>
</html>
