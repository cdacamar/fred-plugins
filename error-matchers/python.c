#include "lex-helpers.h"

typedef struct SMCLIPythonPathResult {
  String8 path;
  String8 line;
} SMCLIPythonPathResult;

SMCLIPythonPathResult smcli_python_match_path_line(String8 line_txt) {
  SMCLIPythonPathResult result = {0};
  // We're trying to match:
  // File "<path>", line <line>
  String8 file_txt      = {0};
  String8 quote_1       = {0};
  String8 path_txt      = {0};
  String8 quote_2       = {0};
  String8 comma_1       = {0};
  String8 line_name_txt = {0};
  String8 line_num_txt  = {0};
  uint64_t start = 0;
  uint64_t end = 0;
  // Eat any spaces.
  end = lex_skip_pred(line_txt, end, lex_is_space);
  start = end;
  // 'File'.
  if (end < line_txt.size) {
    if (lex_is_alpha(line_txt.str[end])) {
      ++end;
      end = lex_skip_pred(line_txt, end, lex_is_alpha);
      file_txt = str8_substr(line_txt, .off = start, .len = end - start);
      if (!lex_is_string_match(file_txt, str8_lit("File"))) {
        file_txt = (String8){0};
      }
      else {
        // We have a successful 'File'.  Let's eat trailing whitespace.
        end = lex_skip_pred(line_txt, end, lex_is_space);
      }
      start = end;
    }
  }

  // '"' (first quote).
  if (file_txt.size != 0 && end < line_txt.size) {
    if (line_txt.str[end] == '"') {
      // Skip the '"'.
      ++end;
      quote_1 = str8_substr(line_txt, .off = start, .len = end - start);
      start = end;
    }
  }

  // Parse until we hit a '"'.
  // This could break paths that have quotes but... w.e...
  for (;end < line_txt.size; ++end) {
    if (line_txt.str[end] == '"') {
      path_txt = str8_substr(line_txt, .off = start, .len = end - start);
      break;
    }
  }
  start = end;

  // Enclosing '"'.
  if (path_txt.size != 0 && end < line_txt.size) {
    if (line_txt.str[end] == '"') {
      // Skip the '"'.
      ++end;
      // Eat any whitespace.
      end = lex_skip_pred(line_txt, end, lex_is_space);
      quote_2 = str8_substr(line_txt, .off = start, .len = end - start);
      start = end;
    }
  }

  // ','.
  if (quote_2.size != 0 && end < line_txt.size) {
    if (line_txt.str[end] == ',') {
      // Skip the '"'.
      ++end;
      // Eat any whitespace.
      end = lex_skip_pred(line_txt, end, lex_is_space);
      comma_1 = str8_substr(line_txt, .off = start, .len = end - start);
      start = end;
    }
  }

  // 'line'.
  if (comma_1.size != 0 && end < line_txt.size) {
    if (lex_is_alpha(line_txt.str[end])) {
      ++end;
      end = lex_skip_pred(line_txt, end, lex_is_alpha);
      line_name_txt = str8_substr(line_txt, .off = start, .len = end - start);
      if (!lex_is_string_match(line_name_txt, str8_lit("line"))) {
        line_name_txt = (String8){0};
      }
      else {
        // Also eat trailing whitespace.
        end = lex_skip_pred(line_txt, end, lex_is_space);
      }
      start = end;
    }
  }

  // <line>
  if (line_name_txt.size != 0 && end < line_txt.size) {
    if (lex_is_digit(line_txt.str[end])) {
      ++end;
      end = lex_skip_pred(line_txt, end, lex_is_digit);
      line_num_txt = str8_substr(line_txt, .off = start, .len = end - start);
    }
  }

  result.path = path_txt;
  result.line = line_num_txt;
  return result;
}

void smcli_output_matcher_python(SMCLICtx* ctx, SMCLIMatcherResult* result, String8 output) {
  // The results from python look something like:
/*
  File "D:\git_projects\fred\test.py", line 1
    print ""
    ^^^^^^^^
SyntaxError: Missing parentheses in call to 'print'. Did you mean print(...)
*/
  // So what we will try to match for paths are:
  // "File "<path>", line <line>
  // For diagnostics, we need to find the next line which is not prefixed with a ' '.
  // Further, we will mark the diagnostic lines as decoration targets.
  uint64_t start = 0;
  uint64_t end = 0;
  String8 line_txt = {0};
  SMCLIPythonPathResult working_line = {0};
  // Our strategy will be to split into lines and scan for the line number result and
  // then the diagnostic line.
  for (; end < output.size; ++end) {
    if (output.str[end] == '\n') {
      line_txt = str8_substr(output, .off = start, .len = end - start);
      if (working_line.path.size == 0 || working_line.line.size == 0) {
          working_line = smcli_python_match_path_line(line_txt);
      }
      // Otherwise, look for a line without a leading space.
      else if (line_txt.size != 0) {
        if (line_txt.str[0] != ' ') {
            // Make this line our diagnostic.
            SMCLIMatcherClickablePath click_path = {0};
            click_path.path.off = working_line.path.str - output.str;
            click_path.path.size = working_line.path.size;
            click_path.diag.off = line_txt.str - output.str;
            click_path.diag.size = line_txt.size;
            click_path.line = u64_from_str8(working_line.line, 10);
            click_path.sev_cat = SMCLI_MATCHER_CAT_Error;
            smcli_add_clickable_path(ctx, result, &click_path);
            // Decorate the line.
            SMCLIMatcherDecoration dec_txt = {0};
            // Same location as the diag.
            dec_txt.string = click_path.diag;
            dec_txt.sev_cat = click_path.sev_cat;
            smcli_add_text_decoration(ctx, result, &dec_txt);
            // Clear working path.
            working_line = (SMCLIPythonPathResult){0};
        }
      }
      else {
        // Clear working path.
        working_line = (SMCLIPythonPathResult){0};
      }
      // The start should be past this newline.
      start = end + 1;
    }
  }
}