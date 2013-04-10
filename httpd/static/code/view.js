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
var navigationPane;
var listingPane;
var subscription;

function resizePanes() {
    var windowHeight = ($(window).height() - HEADER_HEIGHT);
    navigationPane.css({ height: windowHeight + "px" });
    listingPane.css({ height: windowHeight + "px" });
    $("body").css({ 'min-height': $(window).height() + "px" });
}

function updateNavigation(data) {
    s = "subscription.folders:";
    for (x in subscription.folders) {
        s += "<br/>\n    [" + x + "] ";
        for (y in subscription.folders[x]) {
            var v = subscription.folders[x][y];
            if (v == null) v = "<i>null</i>";
            s += v + " :: ";
        }
        s = s.substr(0, s.length - 4);
    }
    s += "<br/>\n<br/>\nsubscription.feeds:";
    for (x in subscription.feeds) {
        s += "<br/>\n    [" + x + "] ";
        for (y in subscription.feeds[x]) {
            var v = subscription.feeds[x][y];
            if (v == null) v = "<i>null</i>";
            s += v + " :: ";
        }
        s = s.substr(0, s.length - 4);
    }
    navigationPane.replaceWith("<div id=\"navigation\">" + s + "</div>");
    navigationPane = $("#navigation");
    resizePanes();
    //alert(s);
}

$(function () {
    HEADER_HEIGHT = $("#topbar").height();
    navigationPane = $("#navigation");
    listingPane = $("#listing");

    window.onresize = resizePanes;
    resizePanes();

    $.ajax("/data/api?op=subscription").done(function (data, textStatus, xhr) {
        subscription = data;
        updateNavigation();
        $("#startup").css({ display: "none" });
        navigationPane.css({ display: "block" });
        listingPane.css({ display: "block" });
    }).error(function (xhr, testStatus, error) {
        $("#startup").css({ display: "none" });
        navigationPane.css({ display: "block" });
        listingPane.css({ display: "block" });
    });
});
