    td = getenv("TMPDIR"); if (td == 0 || td[0] == 0) td = getenv("TEMP");   /* TEMP: Windows, as the forwarding unit */
    if (td == 0 || td[0] == 0) td = "/tmp";
