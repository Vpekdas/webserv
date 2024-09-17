<?php
    session_start();

    if (!isset($_SESSION["username"]))
    {
?>
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
<?php
    } else
    {
?>
        <p>Welcome <?= $_SESSION["username"] ?> </p>
<?php
    }
?>
