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
var LNG_VIEW_ALL_ITEMS;
var LNG_VIEW_SUBSCRIPTIONS;
var $navigation;
var $allItems;
var $subscriptions;
var listingPane;
var rootFolder;

function resizePanes() {
    var windowHeight = ($(window).height() - HEADER_HEIGHT);
    $navigation.css({ height: windowHeight + "px" });
    listingPane.css({ height: windowHeight + "px" });
    $("body").css({ 'min-height': $(window).height() + "px" });
}

function navTitle($e, title, unread) {
    var $text = $(document.createTextNode(title));
    if (unread != 0) {
        var $bold = $(document.createElement("strong"));
        $bold.append($text);
        $text = $(document.createTextNode(" (" + unread + ")"));
        $e.append($bold);
        $e.append($text);
    }
    else
        $e.append($text);
}

function navFolder(folder) {
    var $li = $(document.createElement("li"));
    $li.addClass("feed-folder");
    $li.attr("id", "feed-folder-" + folder.id);

    var $chevron = $(document.createElement("span"));
    $chevron.addClass("folder-chevron");
    $li.append($chevron);

    var $icon = $(document.createElement("span"));
    $icon.addClass("folder-icon");
    $li.append($icon);

    navTitle($li, folder.title, folder.unread);

    //append menu
    $li.append(navList(folder));
    return $li;
}

function navFeed(feed, parent) {
    var $li = $(document.createElement("li"));
    $li.addClass("feed-link");
    $li.attr("id", "feed-" + parent + "-" + feed.id);

    var $icon = $(document.createElement("span"));
    $icon.addClass("feed-icon");
    $li.append($icon);

    navTitle($li, feed.title, feed.unread);

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

function toggleChevron() {
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
        if (rootId != -1) {
            var $chevron = $(document.createElement("span"));
            $chevron.addClass("section-chevron");
            $subscriptions.append($chevron);

            var $span = $(document.createElement("span"));
            $span.addClass("title");
            $span.append($(document.createTextNode(LNG_VIEW_SUBSCRIPTIONS)));
            $subscriptions.append($span);
            $subscriptions.append($(navList(data.folders[rootId])));
            $allItems.empty();
            navTitle($allItems, LNG_VIEW_ALL_ITEMS, data.folders[rootId].unread);

            $(".section-chevron", $navigation).each(toggleChevron);
            $(".folder-chevron", $navigation).each(toggleChevron);

            $("span.title", $subscriptions).each(function () {
                $(this).css({ cursor: "pointer" });
                $(this).click(function () {
                    $(".section-chevron", $(this).parent()).each(function () {
                        $(this).toggleClass("closed");
                        return false;
                    });
                    $("ul", $(this).parent()).each(function () {
                        $(this).toggleClass("closed");
                        return false;
                    });
                    ev.stopImmediatePropagation();
                });
                return false;
            });
        }
        //alert(s);
    } catch (e) { alert(e); }
}

$(function () {
    HEADER_HEIGHT = $("#topbar").height();
    $navigation = $("#navigation");
    listingPane = $("#listing");

    $allItems = $("#all-items", $navigation);
    $subscriptions = $("#subscriptions", $navigation);
    LNG_VIEW_ALL_ITEMS = $allItems.text();
    LNG_VIEW_SUBSCRIPTIONS = $subscriptions.text();

    window.onresize = resizePanes;
    resizePanes();

    $.ajax("/data/api?op=subscription").done(function (data, textStatus, xhr) {
        updateNavigation(data);
        $("#startup").css({ display: "none" });
        $navigation.css({ display: "block" });
        listingPane.css({ display: "block" });
    }).error(function (xhr, testStatus, error) {
        $("#startup").css({ display: "none" });
        $navigation.css({ display: "block" });
        listingPane.css({ display: "block" });
    });
});
