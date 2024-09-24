
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
    <link rel="stylesheet" href="index.css" />
</head>
<body>
<?php
    session_start();

    if (!isset($_SESSION["username"]))
    {
?>
    <div class="login-form">
        <h1>Login</h1>

        <form action="login.php" method="POST">
            <div>
                Username:
                <input type="text" name="username" />
            </div>
            <div>
                Password:
                <input type="password" name="password" />
            </div>
            <input type="submit" name="submit" value="Login" />
        </form>
    </div>
<?php
    } else
    {
?>
        <p>Welcome <?= $_SESSION["username"] ?> <a href="/logout.php">Logout</a></p>
<?php
    }
?>
</body>
</html>
