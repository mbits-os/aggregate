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

function setupPopup(name) {
    try {
        var $popup = $("li#" + name);
        if ($popup.attr("id") != name) // auth page - no user popup
            return;
        var $link = null;
        var $div = null;
        $("a.bar-item", $popup).each(function (index, domElem) {
            if ($link == null) {
                $link = $(domElem);
                return false;
            }
        });
        $("div.tb-sub-wrapper", $popup).each(function (index, domElem) {
            if ($div == null) {
                $div = $(domElem);
                return false;
            }
        });


        $link.removeAttr("href");
        $link.css({ cursor: "pointer" });
        $link.click(function () {
            $popup.toggleClass("visible-popup-item");
            $div.toggleClass("visible-popup");
        });
    } catch (e) { alert(e); }
}

function setupCommand(id, fn) {
    $cmd = $("#" + id);
    if ($cmd.length == 0)
        return;
    $a = $("a", $cmd);
    $a.removeAttr("href");
    $cmd.click(fn);
    $cmd.css({ cursor: "pointer" });
}

var dimmer = null;

function Dimmer() {
    this.catcher = $("#mouse-catcher");
    this.catcher.click(function (ev) {
        dimmer.toggle();
        dimmer.current = null;
        ev.stopImmediatePropagation();
    });
    this.current = null;

    this.show = function (jq) {
        this.current = jq;
        this.toggle();
    }

    this.toggle = function () {
        this.catcher.toggleClass("show-dlg");
        if (this.current != null)
            this.current.toggleClass("show-dlg");
    }
};

function setupDialog(id, callbacks) {
    $("form", $("#" + id)).submit(callbacks, function (ev) {
        if (ev.data[ev.relatedTarget.name])
            ev.data[ev.relatedTarget.name]();
        return false;
    });
}

var subscribeXHR = null;
function subscribe() {
    var $dlg = $("#subscribe");
    var $input = $("#subscribe-url");
    var $btn = $("input[type='submit']:first", $dlg);
    $input.attr("disabled", "disabled");
    $btn.attr("disabled", "disabled");

    if (subscribeXHR != null) {
        subscribeXHR.abort();
        subscribeXHR = null;
    }

    subscribeXHR = $.ajax({
        url: "/data/api",
        data: { op: "subscribe", url: $input.val() },
        dataType: "json"
    }).done(function (data) {
        $(".subscribe-error", $dlg).text("");
        dimmer.toggle();

        var $target = $("#feed-" + data.folder + "-" + data.feed);
        if ($target.length != 0) {
            $("a", $target).trigger("click");
        } else {
            alert(subscribeXHR.responseText);
        }
        subscribeXHR = null;
    }).error(function (xhr) {
        // ERROR HANDLER
        var data = { error: null };
        try {
            var data = JSON.parse(subscribeXHR.responseText);
        } catch (e) { }
        if (!data.error) {
            data.error = LNG_SUBSCRIBE_ERROR;
        }
        $(".subscribe-error", $dlg).text(data.error);
        $input.removeAttr("disabled");
        $input.focus();
        subscribeXHR = null;
    });
}

$(function () {
    dimmer = new Dimmer();
    setupPopup("topbar-menu-user");
    setupCommand("topbar-menu-new", function () {
        var $dlg = $("#subscribe");
        var $input = $("#subscribe-url");
        var $btn = $("input[type='submit']:first", $dlg);
        $(".subscribe-error", $dlg).text("");
        $input.val("");
        dimmer.show($dlg);
        $input.removeAttr("disabled");
        $input.focus();
        $input.keyup(function () {
            var val = $(this).val();
            if (val)
                $btn.removeAttr("disabled");
            else
                $btn.attr("disabled", "disabled");
        }).keyup();
    });
    setupDialog("subscribe", {
        add: subscribe,
        cancel: function () { dimmer.toggle(); }
    });
});
