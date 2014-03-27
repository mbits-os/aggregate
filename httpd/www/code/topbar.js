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
var dimmer2 = null;

function Dimmer(id, callbackName) {
    this.catcher = $("#" + id);
	this.callbackName = callbackName;
	if (!this.callbackName)
		this.callbackName = "cancel";

	var p = this;
    this.catcher.click(function (ev) {
        p.cancel();
        p.current = null;
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
	
	this.cancel = function () {
		if (this.current == null)
		{
			this.toggle();
			return;
		}
		
		var $btn = $("input[type=submit][name=" + this.callbackName + "]", this.current);
		$btn.trigger("click");
	}
};

function setupDialog(id, callbacks) {
    var $form = $("form", $("#" + id));
    try {
		$("input:submit", $form).bind("click keypress", function () {
			$form.data("callerid", this.name);
		});
	} catch (e) { alert(e); }

    $form.submit(callbacks, function (ev) {
        try {
            var callerId = $form.data("callerid");
            if (callerId && ev.data[callerId])
                ev.data[callerId]();
            ev.preventDefault();
        } catch (e) { alert(e); }
        return false;
    });
}

var manyIndex = 0;
function subscribeMultiSubmitEnable(){
	var selectCount = 0;
	$("input[type=checkbox]", $("#subscribe-many-container")).each(function (index, dom){
		var val = dom.checked;
		if (val)
			selectCount++;
	});

	var $btn = $("input[type='submit']:first", $("#subscribe-many"));
	if (selectCount > 0)
		$btn.removeAttr("disabled");
	else
		$btn.attr("disabled", "disabled");
}

function subscribeToMany(links)
{
    var $dlg = $("#subscribe-many");
	var $container = $("#subscribe-many-container");
    var $btn = $("input[type='submit']:first", $dlg);

	$container.html("");
    $btn.attr("disabled", "disabled");
	$("#subscribe-spinner").remove();

	for (i = 0; i < links.length; ++i)
	{
		var inputId = "subscribe-" + manyIndex;
		manyIndex++;

		var $link = $("<div><input type='checkbox' id='" + inputId + "'></div>");
		$link.attr("ref", links[i].href);
		$link.attr("title", links[i].href);
		var $label = $("<label>");
		$link.append($label);
		$label.attr("for", inputId);

		var title = links[i].title;
		if (!title)
			title = LNG_VIEW_TITLE_MISSING;
		$label.text(title);

		if (links[i].comment)
		{
			var comment = " " + links[i].comment;
			var $comment = $("<em style='color: #444'></em>");
			$comment.text(comment);
			$label.append($comment);
		}

		$container.append($link);

		$("#" + inputId).change(subscribeMultiSubmitEnable);
	}

	dimmer2.show($dlg);
}

var subscribeManyXHR = null;
function subscribeMany()
{
    var $dlg = $("#subscribe-many");
	var $container = $("#subscribe-many-container");
	var $inputs = $("input[type=checkbox]", $container);
    var $btn = $("input[type='submit']:first", $dlg);
    var $btns = $btn.parent();

	var links = new Array();
	$inputs.each(function(index, dom) {
		if (dom.checked)
		{
			links.push($(dom).parent().attr("ref"));
		}
	});
	var json = JSON.stringify(links);

    $inputs.attr("disabled", "disabled");
    $btn.attr("disabled", "disabled");

	alert(json);
    if (subscribeManyXHR != null) {
        subscribeManyXHR.abort();
        subscribeManyXHR = null;
    }

    var $loader = $("<span></span>");
    $loader.addClass("loader");
    $loader.attr("id", "subscribe-spinner");
    $btns.prepend($loader);

    subscribeXHR = $.ajax({
        url: "/data/api?op=subscribe-many",
        data: { json: json },
		type: "POST",
        dataType: "json"
    }).done(function (data) {
        $(".subscribe-error", $dlg).text("");
		
		var index = 0;
		var copy = data;
		var then = function() {
			if (index >= copy.length)
			{
				dimmer2.toggle();
				dimmer.toggle();
				return;
			}

			var data = copy[index];
			++index;
			var id = "#feed-" + data.folder + "-" + data.feed;
			var $target = $(id);
			if ($target.length != 0) {
				$("a", $target).trigger("click");
				dimmer.toggle();
			} else {
				updateNavigation(id, function () { then(); });
			}
		};
		
		then();
		$loader.remove();
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
        $loader.remove();
        $(".subscribe-error", $dlg).text(data.error);
        $inputs.removeAttr("disabled");
		subscribeMultiSubmitEnable();
        subscribeXHR = null;
    });

}

var subscribeXHR = null;
function subscribe() {
    var $dlg = $("#subscribe");
    var $input = $("#subscribe-url");
    var $btn = $("input[type='submit']:first", $dlg);
    var $btns = $btn.parent();
    $input.attr("disabled", "disabled");
    $btn.attr("disabled", "disabled");

    if (subscribeXHR != null) {
        subscribeXHR.abort();
        subscribeXHR = null;
    }

    if (subscribeManyXHR != null) {
        subscribeManyXHR.abort();
        subscribeManyXHR = null;
    }

    var $loader = $("<span></span>");
    $loader.addClass("loader");
    $loader.attr("id", "subscribe-spinner");
    $btns.prepend($loader);

    subscribeXHR = $.ajax({
        url: "/data/api",
        data: { op: "subscribe", url: $input.val() },
        dataType: "json"
    }).done(function (data) {
        $(".subscribe-error", $dlg).text("");
		
		if ("links" in data)
		{
			if (!(data.links instanceof Array))
			{
				$loader.remove();
				$(".subscribe-error", $dlg).text(LNG_SUBSCRIBE_ERROR);
				$input.removeAttr("disabled");
				$input.focus();
				subscribeXHR = null;
				return;
			}

			subscribeToMany(data.links);
			$loader.remove();
			return;
		}

        var id = "#feed-" + data.folder + "-" + data.feed;
        var $target = $(id);
        if ($target.length != 0) {
            $("a", $target).trigger("click");
            dimmer.toggle();
        } else {
            updateNavigation(id, function () { dimmer.toggle(); });
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
        $loader.remove();
        $(".subscribe-error", $dlg).text(data.error);
        $input.removeAttr("disabled");
        $input.focus();
        subscribeXHR = null;
    });
}

$(function () {
    dimmer = new Dimmer("mouse-catcher");
    dimmer2 = new Dimmer("mouse-catcher-2");
    setupPopup("topbar-menu-user");
    setupCommand("topbar-menu-new", function () {
        var $dlg = $("#subscribe");
        var $input = $("#subscribe-url");
        var $btn = $("input[type='submit']:first", $dlg);
        $("#subscribe-spinner").remove();
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
    setupDialog("subscribe-many", {
        add: subscribeMany,
        cancel: function () { dimmer2.toggle(); dimmer.toggle(); }
    });
    setupCommand("topbar-menu-refesh", function () {
        updateNavigation();
    });
});
