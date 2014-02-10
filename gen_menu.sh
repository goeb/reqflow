#!/bin/sh

usage() {
    echo "usage: gen_menu.sh --header <h> --footer <f> --page <p> -- <p1> <p2> ..."
    echo ""
    echo "Generate an HTML page from a header, footer, main contents page, and"
    echo "generate a menu that links to other pages <p1> <p2> ..."
    echo ""
    echo "    --page <p>     Select file <p> for the main contents of the page."
    echo ""
    echo "    <p1> <p2> ...  All the pages used for generating the menu."
    echo "                   The page <p> should generally be in this list."
    exit 1
}

get_title() {
    file="$1"
    extension=`echo $file | sed -e "s/.*\.//"`
    if [ "$extension" != "md" ]; then
        echo "Unsupported extension '$extension' (supported: md)"
        exit 1
    fi
    TITLE=`grep -m 1 "^#" "$file" | pandoc | grep "^<h1" | sed -e "s;</h1>;;" -e "s;.*>;;"`
    printf "$TITLE"
}

# generate menu with entries h2 in the same page
generate_menu_L2() {
    currentFile="$1"
    echo "<ul class="menu_level2">"
    grep "^##[^#]" $currentFile | pandoc | while read h2; do
        anchor=`echo $h2 | sed -e 's/.*id="//' -e 's/".*//'`
        title=`echo $h2 | sed -e "s;</h2>;;" -e "s;.*>;;"`
        echo "<li><a href='#$anchor'>$title</a></li>"
    done
    echo "</ul>"
}

# generate_menu current-page all-pages ...
generate_menu() {
    currentFile="$1"
    shift

    echo "<aside class='menu'>"
    echo "<ul>"
    while [ "$1" != "" ]; do
        file="$1"

        # get the title from the first # mark in the file

        TITLE=`get_title "$file"`
        htmlfile=`echo $file | sed -e "s/\.md$/.html/"`
        if [ "$file" = "$currentFile" ]; then
            echo "<li class='active'>$TITLE"
            # generate level 2 menu (links to ids in the same page)
            generate_menu_L2 "$currentFile"
            echo "</li>"
        else
            echo "<li><a href='$htmlfile'>$TITLE</a></li>"
        fi
        shift
    done

    echo "</ul>"
    echo "</aside>"
}

# main ##################

while [ "$1" != "--" -a "$1" != "" ]; do
    case "$1" in
        --header) HEADER="$2"; shift 2;;
        --footer) FOOTER="$2"; shift 2;;
        --page) PAGE="$2"; shift 2;;
    esac
done

shift # remove the --

if [ -z "$HEADER" ]; then usage; fi
if [ -z "$FOOTER" ]; then usage; fi
if [ -z "$PAGE" ]; then usage; fi

# start outputting HTML

TITLE=`grep -m 1 "^#" "$PAGE" | pandoc | grep "^<h1" | sed -e "s;</h1>;;" -e "s;.*>;;"`
sed -e "s;__TITLE__;$TITLE;g" < "$HEADER"

generate_menu "$PAGE" $@

# generate contents
echo "<div class='contents'>"
pandoc < $PAGE
echo "</div>"

sed -e "s;__DATE__;`date '+%e %b %Y'`;" < "$FOOTER"
