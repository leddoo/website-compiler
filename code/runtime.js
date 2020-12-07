function is_dom_element(e) {
    return e instanceof Element || e instanceof HTMLDocument;
}

class Tree_Node {
    constructor(dom, name, parent, type) {
        this.Dom    = dom;
        this.Name   = name;
        this.Parent = parent;
        this.Type   = type || null;

        console.assert(!('tree_node' in dom));
        dom.tree_node = this;

        if(parent) {
            console.assert(!(name in parent));
            parent[name] = this;
        }
    }
}


function tn_remove(me) {
    tn_remove_child(me.Parent, me.Name);
}

function tn_remove_child_maybe(me, x) {
    if(typeof x === "string") {
        console.assert(typeof me === "Tree_Node");

        let name = x;
        if(!(name in me)) {
            return false;
        }

        // NOTE(llw): Remove from dom. (Parent may not be me.Dom!)
        let dom = me[name].Dom;
        dom.parentNode.removeChild(dom);

        // NOTE(llw): Remove from tree.
        delete me[name];

        return true;
    }
    else if(typeof x === "Tree_Node") {
        return tn_remove_child_maybe(me, x.Name);
    }

    console.assert(false);
    return false;
}

function tn_remove_child(me, x) {
    let removed = tn_remove_child_maybe(me, x);
    console.assert(removed);
}


function tn_insert_maybe(me, node, x) {
    if(is_dom_element(x) || typeof x === "undefined") {
        console.assert(typeof me   === "Tree_Node");
        console.assert(typeof node === "Tree_Node");
        console.assert(node.Parent === null);
        console.assert(node.Type   !== "Page");

        if(node.Name in me) {
            return false;
        }

        let dom_parent = me.Dom;
        let dom_target = x;
        if(typeof x === "undefined") {
            // NOTE(llw): Append to me.Dom.
            dom_target = null;
        }
        else {
            // NOTE(llw): Parent may not be me.Dom!
            dom_parent = x.parentNode;
        }

        // NOTE(llw): Add to dom.
        dom_parent.insertBefore(node.Dom, dom_target);

        // NOTE(llw): Add to tree.
        me[node.Name] = node;
        node.Parent = me;

        return true;
    }
    else if(typeof x === "string") {
        console.assert(typeof me === "Tree_Node");

        if(!(x in me)) {
            return false;
        }

        return tn_insert_maybe(me, node, me[x].Dom);
    }
    else if(typeof x === "Tree_Node") {
        return tn_insert_maybe(me, node, x.Dom);
    }

    console.assert(false);
    return false;
}

function tn_insert(me, node, x) {
    let inserted = tn_insert_maybe(me, node, x);
    console.assert(inserted);
}

