AI Usage Report (Phase 1)

Tool Used: Google Gemini

Prompt Given: I explained my C `Report` struct to the AI—specifically pointing out that it had an `int` for severity, a `time_t` for the timestamp, and standard C strings for the category and inspector name. Then, I asked it to generate two specific helper functions for my filter command:
1. `int parse_condition(const char *input, char *field, char *op, char *value);`
2. `int match_condition(Report *r, const char *field, const char *op, const char *value);`

What was generated:
The AI came back with a solution for `parse_condition`. Instead of writing a complicated loop to manually slice up the string, it used `sscanf` with a format trick (`"%[^:]:%[^:]:%s"`) to automatically extract everything between the colons. For `match_condition`, it generated a solid block of `if/else` statements to take the extracted strings to the correct struct fields.

What I changed/learned:
The project hints specifically warned us to watch out for how the AI handles type conversions, so I went through the `match_condition` logic carefully. It correctly used `atoi()` to turn the severity string into a real integer, and `atol()` to cast the timestamp. Because it did this, the greater-than and less-than operators evaluated the actual numbers, rather than accidentally doing weird character comparison. 