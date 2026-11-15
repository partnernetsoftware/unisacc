    td = getenv("TMPDIR"); if (td == 0 || td[0] == 0) td = getenv("TEMP");
    if (td == 0 || td[0] == 0) td = "/tmp";
