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

(function ($) {
	function ajax_click(event)
	{
		$.pjax.click(event, {
			container: "#dump",
			headers: { "X-Ajax-Fragment": $(this).attr("x-ajax-fragment") }
		});
	}
	
	function ajax_block_attach(name)
	{
		var $block = $("#" + name);
		var $cont = $("#" + name + "-container");
		$cont.css({position: "relative"});
		$block.css({position: "absolute"});
		$block.offset($cont.offset());
		$block.width($cont.innerWidth());
		$cont.innerHeight($block.height());
	}

	function ajax_block_replace(name)
	{
		var $block = null;
		$("#dump section").each(function () {
			if ($(this).attr("id") == name)
			{
				$block = $(this);
			}
		});
		
		if ($block == null)
			return;

		// remove both from #dump
		$block.parent().detach();
		$block.detach();

		$target = $("#" + name);
		$target.attr("id", name + "-target");
		$cont = $target.parent();

		$block.css({position: "absolute"});
		$block.offset({left:0,top:0});
		$block.width($cont.innerWidth());
		$block.css({opacity: 0});
		$target.before($block);

		$cont.animate({height: $block.height()}, { queue: false });
		$target.animate({opacity: 0}, {
			always: function ()
			{
			$("#" + name + "-target").remove();
			},
			queue: false
		});
		$block.animate({opacity: 1}, { queue: false });
	}

	function ajax_replace(event, xhr)
	{
		var icicle = xhr.getResponseHeader("X-Debug-Icicle");
		var $frozen = $("a.frozen-icon");
		if (icicle == null)
		{
			$frozen.css({display: "none"});
		}
		else
		{
			$frozen.css({display: "block"});
			$frozen.attr("href", "/debug/?frozen=" + icicle); // TODO: escape the icicle value
		}
		//alert(s);
		ajax_block_replace("pages");
		ajax_block_replace("messages");
		ajax_block_replace("controls");
		ajax_block_replace("buttons");
		$("#dump").html("");
	}

	window.onload = function () {
		if (!$.support.pjax)
			return;

		ajax_block_attach("pages");
		ajax_block_attach("messages");
		ajax_block_attach("controls");
		ajax_block_attach("buttons");
		$("body").append("<div id='dump' style='display: none'></div>");
		$("h1").after("<div id='log'>");
	}

	$(document).on('click', 'a[x-ajax-fragment]', ajax_click);
	$(document).on('pjax:end', ajax_replace);
})(jQuery);
