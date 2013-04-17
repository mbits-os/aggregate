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

/***********************************************************
 *
 *  NavItem
 *
 ***********************************************************/

(function () {

    function inherit(Base, Derived) {
        for (method in Base.prototype)
            Derived.prototype[method] = Base.prototype[method];
    }

    function spanWithClass(klass) { return $e("span").addClass(klass); }

    function NavChevron(klass) {
        this.item = spanWithClass(klass + "-off");
        this.klass = klass;
        this.active = false;
    }

    NavChevron.prototype.toggle = function () {
        this.active = !this.active;
        if (this.active) {
            this.item.attr("class", this.klass);
            this.item.css({ cursor: "pointer" });
            this.item.click(function (ev) {
                $(this).toggleClass("closed");
                //one for "a", second for "li"
                $("ul", $(this).parent().parent()).each(function () {
                    $(this).toggleClass("closed");
                    return false;
                });
                ev.stopImmediatePropagation();
                return false;
            });
        } else {
            this.item.attr("class", this.klass + "-off");
            this.item.css({ cursor: "inherit" });
            this.item.unbind("click");
        }
    }

    ///////////////////////////////////////////////////////////////////////
    //
    // BASE CLASS
    //

    this.NavItem = function (itemClass, idAttr, itemId, chevron, icon) {
        var test = $("#" + idAttr);
        if (test.length != 0) {
            this.item = test;
            this.item.empty();
        } else
            this.item = $e("li");

        this.href = $e("a");
        this.label = $e("span");
        this.id = itemId;

        if (test.length == 0) {
            this.item.addClass(itemClass);
            this.item.attr("id", idAttr);
        }

        this.href.addClass("nav-link");
        this.label.addClass("label");

        this.item.append(this.href);
        this.chevron = new NavChevron(chevron);
        this.href.append(this.chevron.item);

        if (icon != null)
            this.href.append(spanWithClass(icon));

        this.href.append(this.label); // filled later in .setTitle()
    }

    NavItem.selected = null;

    NavItem.prototype.click = function (onselect) {
        if (onselect != null) {
            this.item.on("select", onselect);
            this.href.click(function (ev) {
                var item = $(this).parent();
                if (NavItem.selected != null && NavItem.selected.attr("id") == item.attr("id"))
                    return;

                if (NavItem.selected != null)
                    NavItem.selected.toggleClass("selected");

                NavItem.selected = item;
                NavItem.selected.toggleClass("selected");
                $listing.empty();

                item.trigger("select");
                ev.stopImmediatePropagation();
                return false;
            });
            this.href.css({ cursor: "pointer" });
        } else {
            this.item.off("select");
            this.href.off("click");
            this.href.css({ cursor: "default" });
        }
        return this;
    }

    NavItem.prototype.setTitle = function (title, url, unread) {
        this.label.empty();
        this.title = title;
        this.url = url;
        this.unread = unread;

        var text = $t(title);
        if (unread != 0) {
            var bold = $e("strong");
            bold.addClass("label-text");
            bold.append(text);
            this.label.append(bold);
            count = $e("span");
            count.addClass("unreads");
            count.append($t("(" + unread + ")"));
            this.label.append(count);
        } else {
            var span = $e("span");
            span.addClass("label-text");
            span.append(text);
            this.label.append(span);
        }
        return this;
    }

    ///////////////////////////////////////////////////////////////////////
    //
    // FEED
    //

    this.FeedItem = function (id, parentId) {
        NavItem.call(this, "feed-link", "feed-" + parentId + "-" + id, id, "small-chevron", "feed-icon");
        var item = this;
        this.click(function (ev) {
            showFeed(item.title, item.url, item.id);
            ev.stopImmediatePropagation();
        });
    }
    inherit(NavItem, FeedItem);

    ///////////////////////////////////////////////////////////////////////
    //
    // FOLDER
    //

    this.FolderItem = function (src) {
        var id = src.id;
        NavItem.call(this, "folder-link", "feed-folder-" + id, id, "small-chevron", "folder-icon");
        this.children = $e("ul");
        this.subs = Array();
        this.feeds = Array();
        var item = this;

        this.click(function (ev) {
            showMixed(item.title, item.id);
            ev.stopImmediatePropagation();
        });

        this.item.append(this.children);
    }
    inherit(NavItem, FolderItem);

    FolderItem.prototype.addFeed = function (remote) {
        var feed = new FeedItem(remote.id, this.id);
        feed.setTitle(remote.title, remote.url, remote.unread);
        this.feeds[this.feeds.length] = feed;
        this.children.append(feed.item);

        if (this.feeds.length + this.subs.length == 1)
            this.toggleChevron();
    }

    FolderItem.prototype.addFolder = function (remote) {
        var folder = new FolderItem(remote);
        folder.setTitle(remote.title, null, remote.unread);
        this.subs[this.subs.length] = folder;
        if (this.subs.length == 1)
            this.children.prepend(folder.item);
        else
            this.subs[this.subs.length - 2].item.after(folder.item);

        if (this.feeds.length + this.subs.length == 1)
            this.toggleChevron();
    }

    FolderItem.prototype.merge = function (remote) {
        mergeList.call(this, this.subs, remote.subs, { addItem: FolderItem.prototype.addFolder, removeItem: function () { }, moveItem: function () { } });
        mergeList.call(this, this.feeds, remote.feeds, { addItem: FolderItem.prototype.addFeed, removeItem: function () { }, moveItem: function () { } });
        for (var i in this.subs) {
            this.subs[i].merge(remote.subs[i]);
        }
    }

    FolderItem.prototype.toggleChevron = function () { this.chevron.toggle(); }

    ///////////////////////////////////////////////////////////////////////
    //
    // ROOT
    //

    this.RootItem = function (src) {
        NavItem.call(this, null, "all-items", src.id, "big-chevron");
        this.subscriptions = new NavItem(null, "subscriptions", 0, "big-chevron");
        this.subscriptions.setTitle(LNG_VIEW_SUBSCRIPTIONS, null, 0);
        this.children = $e("ul");
        this.subs = Array();
        this.feeds = Array();
        var item = this;
        this
            .setTitle(src.title, null, src.unread)
            .click(function (ev) {
                showMixed(item.title, item.id);
                ev.stopImmediatePropagation();
            });

        this.subscriptions.item.append(this.children);
    }
    inherit(FolderItem, RootItem);

    RootItem.prototype.toggleChevron = function () { this.subscriptions.chevron.toggle(); }

    /*********************************************************************/

    ///////////////////////////////////////////////////////////////////////
    //
    // MERGING LOCAL AND REMOTE
    //

    function newPosition(list, id) {
        var i = 0;
        for (i in list) {
            if (list[i].id == id)
                return i;
        }
        return -1;
    };

    function nextMove(positions) {
        var index = positions.length;
        for (var i in positions) {
            if (i != positions[i]) {
                if (index > positions[i])
                    index = positions[i];
            }
        }
        return index;
    }

    function findDelta(local, remote, addItem) {
        var locPos = [];
        var rems = [];
        for (var i in local) locPos[i] = -1;
        for (var i in remote) rems[i] = 0;
        for (var i in local) {
            var pos = newPosition(remote, local[i].id);
            locPos[i] = pos;
            if (pos != -1)
                rems[pos] = 1;
        }

        for (var i in rems) {
            if (rems[i] == 0) {
                locPos[locPos.length] = i;
                addItem.call(this, remote[i]);
            }
        }
        return locPos;
    }

    function mergeList(local, remote, cb) {
        var positions = findDelta.call(this, local, remote, cb.addItem);
        var length = positions.length;
        var pos = 0;

        while (pos < length) {
            if (positions[pos] == -1) {
                length--;
                positions.splice(pos, 1);
                cb.removeItem.call(this, pos);
                //local.splice(pos, 1);
            } else {
                pos++;
            }
        }

        while (true) {
            var pos = nextMove(positions);
            if (pos == positions.length)
                break;
            var dst = positions[pos];
            positions.splice(pos, 1);
            positions.splice(dst, 0, dst);

            //var obj = local[pos];
            //local.splice(pos, 1);
            //local.splice(dst, 0, obj);
            cb.moveItem.call(this, pos, dst);
        }

        for (var i in local) {
            var dst = local[i];
            var src = remote[i];
            if (dst.title != src.title || dst.unread != src.unread) {
                dst.setTitle(src.title, src.unread);
            }
        }
        return local;
    };

})();

function buildNavigation(data) {
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

    var root = null;
    if (rootId != -1) {
        root = data.folders[rootId]
        root.title = LNG_VIEW_ALL_ITEMS;
    }
    return root;
}

function createNavigation(data) {
    rootFolder = buildNavigation(data);
    RootItem.singleton = new RootItem(rootFolder);
    RootItem.singleton.merge(rootFolder);
    //$subscriptions.append($(navList(rootFolder)));
}

function getNavigation(then) {
    $.ajax("/data/api?op=subscription")
    .done(function (data, textStatus, xhr) { then(data, xhr, textStatus); })
    .error(function (xhr, textStatus, error) { then(null, xhr, textStatus, error); });
}

function updateNavigation(folderId, then) {
    getNavigation(function (data) {
        if (!data) return;
        var root = buildNavigation(data);
        if (!root) return;
        try {
            RootItem.singleton.merge(root);
            if (folderId != null)
                $("a", $(folderId)).trigger("click");
        } catch (e) { alert(e); }
        if (then != null) then();
    });
}