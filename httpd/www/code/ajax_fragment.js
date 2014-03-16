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
		var $block = $("#" + name + "-xhr");
		
		if ($block.length != 1)
			return;

		$block.detach();

		var $target = $("#" + name);
		$target.attr("id", name + "-target");
		$block.attr("id", name);
		$target.before($block);

		ajax_animate($target, $block);
	}

	function ajax_animate($from, $to)
	{
		$cont = $from.parent();
		$to.css({position: "absolute"});
		$to.offset($from.offset());
		$to.width($from.width());
		$to.css({opacity: 0});
		$from.css({background: "white"});

		$cont.animate({height: $to.height()}, { queue: false, duration: 300 });
		$from.animate({opacity: 0}, {
			always: function () { $from.detach(); },
			queue: false,
			duration: 300
		});
		$to.animate({opacity: 1}, { queue: false, duration: 300 });
	}

	function ajax_block_prepare(name)
	{
		var $section = $("#" + name);
		var $clone = $section.clone();
		$clone.attr("id", name + "-xhr");
		$("#dump").append($clone);
	}

	function ajax_replace(event, xhr)
	{
		var icicle = null;
		if (xhr)
		{
			icicle = xhr.getResponseHeader("X-Debug-Icicle");
			if (icicle != null)
				icicle = "/debug/?frozen=" + icicle // TODO: escape the icicle value
		}
		else
		{
			var $link = $("#dump link[rel=X-Debug-Icicle]");
			icicle = $link.attr("href");
		}

		var $frozen = $("a.frozen-icon");
		if (icicle == null)
		{
			$frozen.css({display: "none"});
		}
		else
		{
			$frozen.css({display: "block"});
			$frozen.attr("href", icicle);
		}

		$.fn.xhr_sections_each(ajax_block_replace);
		$("#dump").html("");
	}

	function ajax_prepare(event)
	{
		$("#dump").html("");
		$.fn.xhr_sections_each(ajax_block_prepare);
		var $frozen = $("a.frozen-icon");
		var node = "<link rel=\"X-Debug-Icicle\" href=\"" + $frozen.attr("href") + "\" />";
		$("#dump").append(node);
	}

	$.fn.extend({
		xhr_sections_each: function(f)
		{
			var count = $.section_names.length;
			for (i = 0; i < count; ++i)
			{
				f($.section_names[i]);
			}
		},

		xhr_sections: function()
		{
			if (!$.support.pjax)
				return;

			$("body").append("<div id='dump' style='display: none'></div>");
			$.section_names = arguments;
			$.fn.xhr_sections_each(ajax_block_attach);
		}
	});

	$(document).on('click', 'a[x-ajax-fragment]', ajax_click);
	$(document).on('pjax:pushstate', ajax_prepare);
	$(document).on('pjax:end', ajax_replace);
	$(document).on('pjax:popstate', ajax_replace);
})(jQuery);
