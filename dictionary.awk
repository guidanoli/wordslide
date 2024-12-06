BEGIN \
{
    word_count = 0
    max_word_length = 0

    print "#include \"dictionary.h\""
    print ""
    print "const char* ws_dictionary[] = {"
}

/^[A-Za-z]*$/ \
{
    word_count++

    if (length > max_word_length)
    {
        max_word_length = length
    }

    printf "    \"%s\",\n", tolower($0)
}

END \
{
    print "};"
    print ""
    printf "const int ws_dictionary_word_count = %d;\n", word_count
    print ""
    printf "const int ws_dictionary_max_word_length = %d;\n", max_word_length
}
