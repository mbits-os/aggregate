/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

var $listing;
var feedXHR = null;

function makeHeader(title, href, whileLoading) {
    var $h = $e("h1");
    $h.addClass("feed-title");
    var $s = $e("span");
    $h.append($s);

    if (href) {
        $a = $e("a");
        $a.attr("href", href);
        $a.attr("target", "_blank");
        $a.append($t(title + " »"));
        $s.append($a);
    } else {
        $s.append($t(title));
    }

    if (whileLoading) {
        $s.addClass("loading-title");
    }

    $listing.append($h);
}

function showMixed(title, id) {
    makeHeader(title);
}

function presentFeed(data) {
    $listing.empty();
    makeHeader(data.title, data.site);
    for (i = 0; i < data.entries.length; ++i) {
        var entry = data.entries[i];
        $entry = $e("div");
        $listing.append($entry);

        $entry.addClass("entry-container");
        $title = $e("div");
        $title.addClass("entry-title");
        $entry.append($title);

        title = "";
        if (entry.title != null)
            title = entry.title;
        if (!title) {
            title = LNG_VIEW_TITLE_MISSING;
        }

        $h = $e("h2");
        if (entry.url != null) {
            $a = $e("a");
            $a.attr("href", entry.url);
            $a.attr("target", "_blank");
            $a.append($t(title));
            $h.append($a);
        } else $h.append($t(title));
        $title.append($h);

        author = "";
        if (entry.author || entry.authorLink) {
            if (entry.author) author = entry.author;
            if (entry.author && entry.authorLink) author += " ";
            if (entry.authorLink) author += entry.authorLink;
        }

        if (author) {
            var $author = $e("span");
            $author.addClass("author");
            $author.append($t(LNG_VIEW_BY));
            $author.append($t(author));
            $title.append($author);
        }

        if (entry.categories.length > 0) {
            var $cats = $e("span");
            $cats.addClass("categories");
            $cats.append($t(LNG_VIEW_PUBLISHED_UNDER));
            for (x = 0; x < entry.categories.length; x++) {
                if (entry.categories.length == 1 || x + 1 < entry.categories.length)
                    $cats.append($t(" "));
                else
                    $cats.append($t(" " + LNG_GENERIC_LIST_LAST + " "));
                var $nobr = $e("nobr");
                $nobr.append(entry.categories[x]);
                var $term = $e("span");
                $term.addClass("term");
                $term.append($nobr);
                $cats.append($term);
            }
            $title.append($cats);
        }

        content = "<div class='content'>";
        if (entry.contents != null)
            content += entry.contents;
        else if (entry.description != null)
            content += entry.description;
        content += "</div>";
        $entry.append($(content));

        if (entry.enclosures.length > 0) {
            var $enc = $e("div");
            $enc.addClass("enclosures");

            var $title = $e("span");
            if (entry.enclosures.length == 1)
                $title.append($t(LNG_VIEW_ENCLOSURE));
            else
                $title.append($t(LNG_VIEW_ENCLOSURES));
            $enc.append($title);

            var $ul = $e("ul");
            for (x = 0; x < entry.enclosures.length; ++x) {
                var enclosure = entry.enclosures[x];
                var $li = $e("li");
                var $a = $e("a");
                $a.attr("href", enclosure.url);
                $a.append($t(enclosure.url));
                $li.append($a);
                $ul.append($li);
                /*if (enclosure.mime.substr(0, 6) == "video/") {
                    var v = document.createElement("video");
                    if (typeof v != "undefined") {
                        if (v.canPlayType(enclosure.mime) != "") {
                            var $v = $(v);
                            $v.attr("src", enclosure.url);
                            $enc.append($v);
                        }
                    }
                } else if (enclosure.mime.substr(0, 6) == "audio/") {
                    var v = document.createElement("audio");
                    if (typeof v != "undefined") {
                        if (v.canPlayType(enclosure.mime) != "") {
                            var $v = $(v);
                            $v.attr("src", enclosure.url);
                            $enc.append($v);
                        }
                    }
                }*/
            }
            $enc.append($ul);
            $entry.append($enc);
        }
    }
}

function showFeed(title, url, id) {
    makeHeader(title, url, true);

    var $loader = $e("div");
    $loader.addClass("loader");
    $listing.append($loader);

    if (feedXHR != null) {
        feedXHR.abort();
        feedXHR = null;
    }

    feedXHR = $.ajax({
        url: "/data/api",
        data: { op: "feed", feed: id },
        dataType: "json",
    }).done(function (data) {
        feedXHR = null;
        presentFeed(data);
    }).error(function (xhr) {
        feedXHR = null;
        $listing.empty();
        makeHeader(title);
    });
}
