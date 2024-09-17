<?php
    session_start();

    if (isset($_POST["password"]) && $_POST["password"] == "fghjkl;'")
    {
        $_SESSION["username"] = $_POST["username"];
        header("Location: /");
    } else
    {
?>
        <div>
            Wrong password!
        </div>
<?php
    }
?>
