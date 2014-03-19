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
	$.custom_name_policy = -1;
	
	function displayName() { return document.getElementById("display_name"); }

	function setDisplayNames()
	{
		var name = $("#name").val();
		var family = $("#family_name").val();
		var display_name = displayName();

		display_name[2].setAttribute("custom", name);
		display_name[3].setAttribute("custom", family);
		display_name[4].setAttribute("custom", name+" "+family);
		display_name[5].setAttribute("custom", family+" "+name);
		display_name[6].setAttribute("custom", name+", "+family);
		display_name[7].setAttribute("custom", family+", "+name);

		if ($.custom_name_policy > 0)
			$("#custom").val($(display_name[$.custom_name_policy]).attr("custom"));
	}

	function setDisplayName()
	{
		var display_name = displayName();
		var $custom = $("#custom");

		if ($.custom_name_policy == display_name.value) return;
		$.custom_name_policy = display_name.value;
		if ($.custom_name_policy == 0)
		{
			$custom.prop('disabled', false);
			return;
		}
		$custom.prop('disabled', true);
		$custom.val($(display_name[$.custom_name_policy]).attr("custom"));
	}

	function checkDisplayName()
	{
		var display_name = displayName();
		var $custom = $("#custom");
		
		var i;
		for (i = 2; i < 8; i++)
		{
			if ($custom.val() == display_name[i].getAttribute("custom"))
			{
				display_name.value = i;
				return;
			}
		}
	}

	setDisplayNames();
	checkDisplayName();
	setDisplayName();
	var $name = $("#name");
	var $family_name = $("#family_name");
	var $display_name = $("#display_name");

	$name.change(setDisplayNames);
	$name.keyup(setDisplayNames);
	$family_name.change(setDisplayNames);
	$family_name.keyup(setDisplayNames);
	$display_name.change(setDisplayName);

})(jQuery);
