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

var $homeLink;
var $allItems;
var $subscriptions;
var rootFolder;
var $selected = null;

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
        var i;

        for (i in data.folders) {
            data.folders[i].unread = 0;
            data.folders[i].subs = new Array();
            data.folders[i].feeds = new Array();

            var parent = data.folders[i].parent;
            data.folders[i].parent = -1;
            if (parent == 0) {
                rootId = i;
            } else {
                var j;
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
            var j;
            for (j in data.folders) {
                if (data.folders[j].id == parent) {
                    data.feeds[i].parent = j;
                    data.folders[j].feeds.push(data.feeds[i]);
                    data.folders[j].unread += data.feeds[i].unread;
                    break;
                }
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
