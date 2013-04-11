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

var HEADER_HEIGHT = -1;
var $navigation;
var $homeLink;
var $allItems;
var $subscriptions;
var $listing;
var rootFolder;
var $selected = null;

function $e(name) { return $(document.createElement(name)); }
function $t(text) { return $(document.createTextNode(text)); }

String.prototype.repeat = function(count) {
    if (count < 1) return '';
    var result = '', pattern = this.valueOf();
    while (count > 0) {
        if (count & 1) result += pattern;
        count >>= 1, pattern += pattern;
    }
    return result;
};

function resizePanes() {
    var windowHeight = ($(window).height() - HEADER_HEIGHT);
    $navigation.css({ height: windowHeight + "px" });
    $listing.css({ height: windowHeight + "px" });
    $("body").css({ 'min-height': $(window).height() + "px" });
}

function makeHeader(title, href) {
    $h = $e("h3");
    $h.addClass("feed-title");
    if (href == null)
        $h.append($t(title));
    else {
        $a = $e("a");
        $a.attr("href", href);
        $a.attr("target", "_blank");
        $a.append($t(title + " »"));
        $h.append($a);
    }

    $listing.append($h);
}

function showMixed(title, id) {
    makeHeader(title);
}

function print(json, depth) {
    if (depth > 3) return "...";
    if (json == null) return "<null>";
    if (typeof json == "string") return json;
    if (typeof json == "boolean") return json ? "true" : "false";
    if (typeof json == "number") return "" + json;
    if (json instanceof Array) {
        var s = "[\n";
        d = "    ".repeat(depth + 1);
        for (i = 0; i < json.length; ++i) {
            s += d + print(json[i], depth + 1) + "\n";
        }
        return s + "    ".repeat(depth) + "]\n";
    }

    var s = "{\n";
    d = "    ".repeat(depth + 1);
    for (x in json) {
        s += d + x + ": " + print(json[x], depth + 1) + "\n";
    }
    return s + "    ".repeat(depth) + "}\n";
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

        $h = $e("h4");
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
            var $author = $e("div");
            $author.addClass("author");
            $author.append($t(LNG_VIEW_BY));
            $author.append($t(author));
            $title.append($author);
        }

        if (entry.categories.length > 0) {
            var $cats = $e("div");
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
            var $ul = $e("ul");
            for (x = 0; x < entry.enclosures.length; ++x) {
                var enclosure = entry.enclosures[x];
                var $li = $e("li");
                var $a = $e("a");
                $a.attr("href", enclosure.url);
                $a.append($t(enclosure.url));
                $li.append($a);
                $ul.append($li);
                if (enclosure.mime.substr(0, 6) == "video/") {
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
                }
            }
            $enc.append($ul);
            $entry.append($enc);
        }
    }
}

function showFeed(title, id) {
    makeHeader(title);
    $listing.append($t("/data/api?" + $.param({ op: "feed", feed: id })));
    $.getJSON("/data/api?" + $.param({ op: "feed", feed: id }))
        .done(function (data, textStatus, xhr) {
            presentFeed(data);
            //$pre = $e("pre");
            //$pre.append($t(print(data, 0)));
            //$listing.append($pre);
            $p = $e("p");
            $p.append($t(xhr.responseText));
            $listing.append($p);
        })
        .error(function (xhr, testStatus, error) {
        });
}

function selectItem(anchor) {
    $next = $(anchor).parent();

    if ($selected == $next)
        return;

    if ($selected != null)
        $selected.toggleClass("selected");
    $selected = $next;
    $selected.toggleClass("selected");
    $listing.empty();

    $next.trigger("select");
}

function navItem($element, title, unread, chevron, icon, onselect) {
    $a = $e("a");
    $a.addClass("nav-link");
    if (onselect != null) {
        $element.on("select", onselect);
        $a.click(function (ev) {
            selectItem(this);
            ev.stopImmediatePropagation();
        });
        $a.css({ cursor: "pointer" });
    }
    if (chevron != null) {
        var $chevron = $e("span");
        $chevron.addClass(chevron);
        $a.append($chevron);
        $chevron.css({ cursor: "pointer" });
        $chevron.click(function (ev) {
            $(this).toggleClass("closed");
            //one for "a", second for "li"
            $("ul", $(this).parent().parent()).each(function () {
                $(this).toggleClass("closed");
                return false;
            });
            ev.stopImmediatePropagation();
        });
    }
    if (icon != null) {
        var $chevron = $e("span");
        $chevron.addClass(icon);
        $a.append($chevron);
    }

    var $text = $t(title);
    if (unread != 0) {
        var $bold = $e("strong");
        $bold.append($text);
        var $label = $e("span");
        $label.append($bold);
        $label.addClass("label");
        $a.append($label);
        $text = $(document.createTextNode("(" + unread + ")"));
    }

    var $tt = $e("span");
    $tt.append($text);
    $tt.addClass("label");
    $a.append($tt);
    $element.append($a);
}

function navFolder(folder) {
    var $li = $e("li");
    $li.addClass("feed-folder");
    $li.attr("id", "feed-folder-" + folder.id);

    navItem($li, folder.title, folder.unread, "folder-chevron", "folder-icon", function (ev) {
        showMixed(folder.title, folder.id);
        ev.stopImmediatePropagation();
    });

    //append menu
    $li.append(navList(folder));
    return $li;
}

function navFeed(feed, parent) {
    var $li = $e("li");
    $li.addClass("feed-link");
    $li.attr("id", "feed-" + parent + "-" + feed.id);

    navItem($li, feed.title, feed.unread, null, "feed-icon", function (ev) {
        showFeed(feed.title, feed.id);
        ev.stopImmediatePropagation();
    });

    //append menu
    return $li;
}

function navList(list) {
    var ul = document.createElement("ul");
    var $ul = $(ul);
    for (i in list.subs) {
        $ul.append(navFolder(list.subs[i]));
    }
    for (i in list.feeds) {
        $ul.append(navFeed(list.feeds[i], list.id));
    }
    return $ul;
}

function toggleSection() {
    $(this).css({ cursor: "pointer" });
    $(this).click(function (ev) {
        $(this).toggleClass("closed");
        $("ul", $(this).parent()).each(function () {
            $(this).toggleClass("closed");
            return false;
        });
        ev.stopImmediatePropagation();
    });
}

function updateNavigation(data) {
    try {
        var rootId = -1;

        for (i in data.folders) {
            data.folders[i].unread = 0;
            data.folders[i].subs = new Array();
            data.folders[i].feeds = new Array();

            var parent = data.folders[i].parent;
            data.folders[i].parent = -1;
            if (parent == 0) {
                rootId = i;
            } else {
                for (j in data.folders) {
                    if (data.folders[j].id == parent) {
                        data.folders[i].parent = j;
                        break;
                    }
                }
            }
        }
        for (i in data.feeds) {
            var parent = data.feeds[i].parent;
            data.feeds[i].parent = rootId;
            for (j in data.folders) {
                if (data.folders[j].id == parent) {
                    data.feeds[i].parent = j;
                    break;
                }
            }
        }

        for (i in data.feeds) {
            var parent = data.feeds[i].parent;
            if (parent != -1) {
                data.folders[parent].feeds.push(data.feeds[i]);
                data.folders[parent].unread += data.feeds[i].unread;
            }
        }

        for (i in data.folders) {
            var parent = data.folders[i].parent;
            if (parent != -1) {
                data.folders[parent].subs.push(data.folders[i]);
                data.folders[parent].unread += data.folders[i].unread;
            }
        }

        $subscriptions.empty();
        $allItems.empty();
        navItem($subscriptions, LNG_VIEW_SUBSCRIPTIONS, 0, "section-chevron");

        if (rootId != -1) {
            rootFolder = data.folders[rootId];
            $subscriptions.append($(navList(rootFolder)));
            navItem($allItems, LNG_VIEW_ALL_ITEMS, data.folders[rootId].unread, null, null, function (ev) {
                showMixed(LNG_VIEW_ALL_ITEMS, rootFolder.id);
                ev.stopImmediatePropagation();
            });
        }
        else
            navItem($allItems, LNG_VIEW_ALL_ITEMS, 0);
    } catch (e) { alert(e); }
}

$(function () {
    HEADER_HEIGHT = $("#topbar").height();
    $navigation = $("#navigation");
    $listing = $("#listing");

    $homeLink = $("#home", $navigation);
    $allItems = $("#all-items", $navigation);
    $subscriptions = $("#subscriptions", $navigation);

    window.onresize = resizePanes;
    resizePanes();
    $homeLink.empty();
    navItem($homeLink, LNG_VIEW_HOME, 0, null, null, function () {
    });

    $.ajax("/data/api?op=subscription").done(function (data, textStatus, xhr) {
        updateNavigation(data);
        $("a", $homeLink).trigger("click");
        $("#startup").css({ display: "none" });
        $navigation.css({ display: "block" });
        $listing.css({ display: "block" });
    }).error(function (xhr, testStatus, error) {
        $("#startup").css({ display: "none" });
        $navigation.css({ display: "block" });
        $listing.css({ display: "block" });
    });
});
