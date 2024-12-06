BEGIN { printf("#include \"dictionary.h\"\n\nconst char* ws_dictionary[] = {\n"); n = 0; }
/^[A-Za-z]*$/ { printf("    \"%s\",\n", tolower($1)); n++; }
END { printf("};\n\nconst int ws_dictionary_length = %d;\n", n); }
