tn_exports = {};

tn_exports["yay_forms"] = {};
tn_exports["yay_forms"].type = "form";
tn_exports["yay_forms"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("form");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        var my_for_prefix = me.tn_dom.id + "-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "name";
            {
                let text = document.createTextNode("Name");
                dom.append(text);
            }
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "text";
            dom.value = "hi";

            let me = new Tree_Node(my_tree_parent, dom, "name");
        }

        var my_dom_parent = dom;
        var my_for_prefix = me.tn_dom.id + "-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "email";
            {
                let text = document.createTextNode("E-Mail");
                dom.append(text);
            }
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "email";
            dom.value = "a@b";

            let me = new Tree_Node(my_tree_parent, dom, "email");
        }

        var my_dom_parent = dom;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);
            dom.classList.add("desktop");
            dom.classList.add("spacer_4");
        }

        var my_dom_parent = dom;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);
            dom.classList.add("mobile");
            dom.classList.add("spacer_3");
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["instantiate_me"] = {};
tn_exports["instantiate_me"].type = "div";
tn_exports["instantiate_me"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "hey_man");

            var my_dom_parent = dom;
            {
                let dom = document.createElement("span");
                my_dom_parent.append(dom);
                {
                    let text = document.createTextNode("hi");
                    dom.append(text);
                }
            }
        }

        var my_dom_parent = dom;
        var my_for_prefix = "page-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "yay";
        }

        var my_dom_parent = dom;
        var my_tree_parent = window.page;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "yay");
        }

        var my_dom_parent = dom;
        var my_for_prefix = me.tn_dom.id + "-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "foo";
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("h1");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "foo");
            {
                let text = document.createTextNode("Title");
                dom.append(text);
            }
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["x1"] = {};
tn_exports["x1"].type = "div";
tn_exports["x1"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);
        dom.classList.add("x0");
        dom.classList.add("x1");

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "x1");
        {
            let text = document.createTextNode("hi");
            dom.append(text);
        }

        return me;
    }
}

tn_exports["list_item"] = {};
tn_exports["list_item"].type = "div";
tn_exports["list_item"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        var my_for_prefix = me.tn_dom.id + "-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "name";
            {
                let text = document.createTextNode("Name");
                dom.append(text);
            }
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "text";

            let me = new Tree_Node(my_tree_parent, dom, "name");
        }

        var my_dom_parent = dom;
        var my_for_prefix = me.tn_dom.id + "-";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "email";
            {
                let text = document.createTextNode("E-Mail");
                dom.append(text);
            }
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "email";

            let me = new Tree_Node(my_tree_parent, dom, "email");
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["my_list"] = {};
tn_exports["my_list"].type = "list";
tn_exports["my_list"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "list");
        me.tn_listify(tn_exports["list_item"].make, -Infinity, +Infinity);
        for(let i = 0; i < 1; i += 1) {
            me.tn_list_insert_new();
        }
        me.tn_list_min = 1;
        me.tn_list_max = 5;

        return me;
    }
}

tn_exports["test_select"] = {};
tn_exports["test_select"].type = "select";
tn_exports["test_select"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("select");
        my_dom_parent.append(dom);

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "select");
        dom.options[dom.options.length] = new Option("a", "0");
        dom.options[dom.options.length] = new Option("b", "1");
        dom.options[dom.options.length] = new Option("c", "c");

        return me;
    }
}

tn_exports["number_input"] = {};
tn_exports["number_input"].type = "input";
tn_exports["number_input"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("input");
        my_dom_parent.append(dom);
        dom.type = "number";
        dom.value = "42";

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "foo");

        return me;
    }
}

tn_exports["date_input"] = {};
tn_exports["date_input"].type = "input";
tn_exports["date_input"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("input");
        my_dom_parent.append(dom);
        dom.type = "date";

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "foo");

        return me;
    }
}

tn_exports["time_input"] = {};
tn_exports["time_input"].type = "input";
tn_exports["time_input"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("input");
        my_dom_parent.append(dom);
        dom.type = "time";

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "foo");

        return me;
    }
}

tn_exports["checkbox_input"] = {};
tn_exports["checkbox_input"].type = "input";
tn_exports["checkbox_input"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("input");
        my_dom_parent.append(dom);
        dom.type = "checkbox";
        dom.checked = 1;

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "foo");

        return me;
    }
}

tn_exports["file_input"] = {};
tn_exports["file_input"].type = "input";
tn_exports["file_input"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("input");
        my_dom_parent.append(dom);
        dom.type = "file";

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "foo");

        return me;
    }
}

tn_exports["new_inputs"] = {};
tn_exports["new_inputs"].type = "div";
tn_exports["new_inputs"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "number";
            dom.value = "42";

            let me = new Tree_Node(my_tree_parent, dom, "number");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "date";

            let me = new Tree_Node(my_tree_parent, dom, "date");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "time";

            let me = new Tree_Node(my_tree_parent, dom, "time");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "checkbox";
            dom.checked = 1;

            let me = new Tree_Node(my_tree_parent, dom, "check");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "file";

            let me = new Tree_Node(my_tree_parent, dom, "file");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("button");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "button");
            {
                let text = document.createTextNode("foo");
                dom.append(text);
            }
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["classes_and_styles"] = {};
tn_exports["classes_and_styles"].type = "div";
tn_exports["classes_and_styles"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);
        dom.style = "width: 50px; height: 50px; background-color: green";
        dom.classList.add("some_class");

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["ids_div"] = {};
tn_exports["ids_div"].type = "div";
tn_exports["ids_div"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = new Tree_Node(my_tree_parent, dom, id !== undefined ? id : "outer");

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "inner");
        }

        var my_dom_parent = dom;
        var my_tree_parent = window.page;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);

            let me = new Tree_Node(my_tree_parent, dom, "global");
        }

        var my_dom_parent = dom;
        {
            let dom = document.createElement("div");
            my_dom_parent.append(dom);
            dom.id = "html";

            var my_dom_parent = dom;
            var my_tree_parent = me;
            {
                let dom = document.createElement("div");
                my_dom_parent.append(dom);

                let me = new Tree_Node(my_tree_parent, dom, "content");
            }
        }

        var my_dom_parent = dom;
        var my_for_prefix = "";
        {
            let dom = document.createElement("label");
            my_dom_parent.append(dom);
            dom.htmlFor = my_for_prefix + "html";
        }

        return me;
    }
}

tn_exports["new_stuff_div"] = {};
tn_exports["new_stuff_div"].type = "div";
tn_exports["new_stuff_div"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        {
            let dom = document.createElement("a");
            my_dom_parent.append(dom);
            dom.style = "font-weight: bold";
            dom.href = "ids.html";
            {
                let text = document.createTextNode("an anchor");
                dom.append(text);
            }
        }

        if(id !== undefined) {
            return me;
        }
    }
}

tn_exports["validation"] = {};
tn_exports["validation"].type = "div";
tn_exports["validation"].make = function(parent, id) {
    console.assert(parent instanceof Tree_Node);

    let dom = parent.tn_dom;
    let me  = parent;

    var my_dom_parent = dom;
    var my_tree_parent = me;
    {
        let dom = document.createElement("div");
        my_dom_parent.append(dom);

        let me = my_tree_parent;
        if(id !== undefined) {
            me = new Tree_Node(my_tree_parent, dom, id);
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("select");
            my_dom_parent.append(dom);
            dom.required = true;

            let me = new Tree_Node(my_tree_parent, dom, "select");
            dom.options[dom.options.length] = new Option("", "");
            dom.options[dom.options.length] = new Option("yes", "yes");
            dom.options[dom.options.length] = new Option("no", "no");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "text";
            dom.minLength = 5;
            dom.maxLength = 7;

            let me = new Tree_Node(my_tree_parent, dom, "text_1");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "text";
            dom.required = true;
            dom.maxLength = 7;

            let me = new Tree_Node(my_tree_parent, dom, "text_2");
        }

        var my_dom_parent = dom;
        var my_tree_parent = me;
        {
            let dom = document.createElement("input");
            my_dom_parent.append(dom);
            dom.type = "text";
            dom.required = true;
            dom.minLength = 2;

            let me = new Tree_Node(my_tree_parent, dom, "text_3");
        }

        if(id !== undefined) {
            return me;
        }
    }
}

