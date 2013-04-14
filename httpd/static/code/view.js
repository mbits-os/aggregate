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
var $listing;

function $e(name) { return $(document.createElement(name)); }
function $t(text) { return $(document.createTextNode(text)); }

function resizePanes() {
    var windowHeight = ($(window).height() - HEADER_HEIGHT);
    $navigation.css({ height: windowHeight + "px" });
    $listing.css({ height: windowHeight + "px" });
    $("body").css({ 'min-height': $(window).height() + "px" });
}

$(function () {
    HEADER_HEIGHT = $("#topbar").height();
    $navigation = $("#navigation");
    $listing = $("#listing");

    $homeLink = $("#home", $navigation);
    $allItems = $("#all-items", $navigation);
    $subscriptions = $("#subscriptions", $navigation);
    $homeLink.empty();
    navItem($homeLink, LNG_VIEW_HOME, 0, null, null, function () { });

    window.onresize = resizePanes;
    resizePanes();

    $.ajax("/data/api?op=subscription").done(function (data, textStatus, xhr) {
        createNavigation(data);
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
