<?php
    session_start();

    if (isset($_POST["password"]) && $_POST["password"] == "fghjkl;'")
    {
        $_SESSION["username"] = $_POST["username"];
        ?>
        <div>
            Login sucessful! Click <a href="/">here</a> to return to the index
        </div>
<?php
    } else
    {
?>
        <div>
            Wrong password!
        </div>
<?php
    }
?>
