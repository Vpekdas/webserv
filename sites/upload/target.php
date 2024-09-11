<?php
    if (!isset($_POST["file"]))
    {
        ?>
            <p>Invalid request</p>
        <?php
    }
    else
    {
        ?>
            <p><?= $_POST["file"] ?></p>
        <?php
    }
?>
