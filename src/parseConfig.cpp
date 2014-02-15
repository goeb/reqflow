/*   Req
 *   Copyright (C) 2014 Frederic Hoerni
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "parseConfig.h"
#include "logging.h"
#include "global.h"

/** Parse a text buffer and return a list of lines of tokens
  *
  * The syntax is:
  * text := line *
  * line := token * [ '\\' '\n' line ] *
  * token := simple-token | double-quote-token | boundary-token
  * simple-token := [^ \t\n\r"] *
  * double-quote-token := '"' [^"] * '"'
  * boundary-token := '<' boundary '\n' any-text '\n' boundary
  */
std::list<std::list<std::string> > parseConfigTokens(const char *buf, size_t len)
{
    std::list<std::list<std::string> > linesOftokens;
    size_t i = 0;
    enum State {
        P_READY,
        P_IN_DOUBLE_QUOTES,
        P_IN_BACKSLASH,
        P_IN_COMMENT,
        P_IN_BACKSLASH_IN_DOUBLE_QUOTES,
        P_IN_BOUNDARY_HEADER,
        P_IN_BOUNDARY
    };
    enum State state = P_READY;
    std::string token; // current token
    std::list<std::string> line; // current line
    std::string boundary;
    std::string boundedText;
    bool tokenPending = false;

    for (i=0; i<len; i++) {
        char c = buf[i];
        switch (state) {
        case P_IN_COMMENT:
            if (c == '\n') {
                if (line.size() > 0) { linesOftokens.push_back(line); line.clear(); }
                state = P_READY;
            }
            break;

        case P_IN_BACKSLASH:
            if (c == '\n') { // new line escaped
                // nothing particular here, continue on next line
                state = P_READY;
            } else if (c == '\r') { // ignore this

            } else {
                tokenPending = true;
                token += c;
                state = P_READY;
            }
            break;

        case P_IN_DOUBLE_QUOTES:
            if (c == '\\') {
                state = P_IN_BACKSLASH_IN_DOUBLE_QUOTES;
            } else if (c == '"') {
                // end of double-quoted string

                state = P_READY;
            } else token += c;
            break;
        case P_IN_BACKSLASH_IN_DOUBLE_QUOTES:
            token += c;
            state = P_IN_DOUBLE_QUOTES;
            break;
        case P_IN_BOUNDARY_HEADER:
            if (c == '\n') {
                state = P_IN_BOUNDARY;
                boundary.insert(0, "\n"); // add a \n at the beginning
                boundedText.clear();
            } else if (isblank(c)) continue; // ignore blanks
            else {
                boundary += c;
            }
            break;
        case P_IN_BOUNDARY:
            // check if boundary was reached
            if (boundedText.size() >= boundary.size()) { // leading \n already added
                size_t bsize = boundary.size();
                size_t offset = boundedText.size() - bsize;
                if (0 == boundedText.compare(offset, bsize, boundary)) {
                    // boundary found
                    // substract the boundary fro the text
                    boundedText = boundedText.substr(0, offset);
                    state = P_READY;
                    line.push_back(boundedText);
                    boundedText.clear();
                    linesOftokens.push_back(line);
                    line.clear();
                    tokenPending = false;
                }
            }
            // accumulate character if still in same state
            if (state == P_IN_BOUNDARY) boundedText += c;

            break;
        case P_READY:
        default:
            if (c == '\n') {
                if (tokenPending) { line.push_back(token); token.clear(); tokenPending = false;}
                if (line.size() > 0) { linesOftokens.push_back(line); line.clear(); }
            } else if (c == ' ' || c == '\t' || c == '\r') {
                // current toke is done (because c is a token delimiter)
                if (tokenPending) { line.push_back(token); token.clear(); tokenPending = false;}
            } else if (c == '#') {
                if (tokenPending) { line.push_back(token); token.clear(); tokenPending = false;}
                state = P_IN_COMMENT;
            } else if (c == '\\') {
                state = P_IN_BACKSLASH;
            } else if (c == '"') {
                tokenPending = true;
                state = P_IN_DOUBLE_QUOTES;
            } else if (c == '<') {
                tokenPending = true;
                state = P_IN_BOUNDARY_HEADER;
                boundary.clear();
            } else if (c == '\r') {
                // ignore
            } else {
                tokenPending = true;
                token += c;
            }
            break;
        }
    }
    // purge remaininig token and line
    if (tokenPending) line.push_back(token);
    if (line.size() > 0) linesOftokens.push_back(line);

    return linesOftokens;
}

// Allocate a buffer (malloc) and load a file into this buffer.
// @return the number of bytes read (that is also the size of the buffer)
//         -1 in case of error
// If the file is empty, 0 is returned and the buffer is not allocated.
// It is up to the caller to free the buffer (if the return value is > 0).
int loadFile(const char *filepath, const char **data)
{
    //LOG_DEBUG("Loading file '%s'...", filepath);
    *data = 0;
    FILE *f = fopen(filepath, "rb");
    if (NULL == f) {
        LOG_DEBUG("Could not open file '%s', %s", filepath, strerror(errno));
        return -1;
    }
    // else continue and parse the file

    int r = fseek(f, 0, SEEK_END); // go to the end of the file
    if (r != 0) {
        LOG_ERROR("could not fseek(%s): %s", filepath, strerror(errno));
        fclose(f);
        return -1;
    }
    long filesize = ftell(f);
    if (filesize > 4*1024*1024) { // max 4 MByte
        LOG_ERROR("loadFile: file '%s'' over-sized (%ld bytes)", filepath, filesize);
        fclose(f);
        return -1;
    }

    if (0 == filesize) {
        // the file is empty
        fclose(f);
        return 0;
    }

    rewind(f);
    char *buffer = (char *)malloc(filesize+1); // allow +1 for terminating null char
    long n = fread(buffer, 1, filesize, f);
    if (n != filesize) {
        LOG_ERROR("fread(%s): short read. feof=%d, ferror=%d", filepath, feof(f), ferror(f));
        fclose(f);
        free(buffer);
        return -1;
    }
    buffer[filesize] = 0;
    *data = buffer;
    fclose(f);
    return n;
}

/** Write a string to a file
  *
  * @return
  *    0 if success
  *    <0 if error
  */
int writeToFile(const char *filepath, const std::string &data)
{
    return writeToFile(filepath, data.data(), data.size());
}

int writeToFile(const char *filepath, const char *data, size_t len)
{
    int result = 0;
    mode_t mode = O_CREAT | O_TRUNC | O_WRONLY;
    int flags = S_IRUSR;
#if defined(_WIN32)
    mode |= O_BINARY;
    flags |= S_IWUSR;
#endif

    std::string tmp = filepath;
    tmp += ".tmp";
    int f = open(tmp.c_str(), mode, flags);
    if (-1 == f) {
        LOG_ERROR("Could not create file '%s', (%d) %s", tmp.c_str(), errno, strerror(errno));
        return -1;
    }

    if (len > 0) {
        size_t n = write(f, data, len);
        if (n != len) {
            LOG_ERROR("Could not write all data, incomplete file '%s': (%d) %s",
                      filepath, errno, strerror(errno));
            return -1;
        }
    }

    close(f);

#if defined(_WIN32)
    _unlink(filepath);
#endif

    int r = rename(tmp.c_str(), filepath);
    if (r != 0) {
        LOG_ERROR("Cannot rename '%s' -> '%s': (%d) %s", tmp.c_str(), filepath, errno, strerror(errno));
        return -1;
    }

    return result;
}
