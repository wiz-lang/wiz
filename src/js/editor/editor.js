(function(globals) {
globals.editor = globals.editor || {};
globals.editor.init = function() {
    var editorPreference = false;
    var aceEditor = null;

    function initAceEditor(mode) {
        if(!aceEditor) {
            aceEditor = ace.edit("aceEditor");
            aceEditor.setTheme("ace/theme/tomorrow_night");
            aceEditor.getSession().setMode("ace/mode/wiz");
            aceEditor.setDisplayIndentGuides(false);
            aceEditor.setShowPrintMargin(false);
            aceEditor.setShowFoldWidgets(false);
        }
    }

    function getSource() {
        var preference = editorPreference || 'text';
        if(preference === 'ace' && aceEditor) {
            source = aceEditor.getValue();
        } else {
            source = document.querySelector('.editor .textarea.' + preference).value;
        }
        return source;
    }

    function switchEditor(newPreference) {
        var editors = document.querySelector('.editor .textarea');
        var source = getSource();

        if(!editorPreference) {
            editorPreference = 'text';
        }

        if(newPreference === 'ace') {
            if(aceEditor) {
                aceEditor.setValue(source, -1);

                var UndoManager = ace.require("ace/undomanager").UndoManager; 
                aceEditor.getSession().setUndoManager(new UndoManager());
            } else {
                newPreference = 'text';
            }
        }
        if(newPreference !== 'ace') {
            if(newPreference === 'text') {
                var newNode = document.createElement('textarea');
                var oldNode = document.querySelector('.editor .textarea.' + newPreference);
                newNode.className = oldNode.className;
                oldNode.parentNode.replaceChild(newNode, oldNode)
            }
            document.querySelector('.editor .textarea.' + newPreference).value = source;
        }
        editorPreference = newPreference;

        var date = new Date();
        date.setTime(date.getTime()+(30*24*60*60*1000));
        document.cookie = 'wiz_editor_preference=' + newPreference + '; expires=' + date.toGMTString() + '; path=/';

        for(var i = 0, items = document.querySelectorAll('.editor .textarea'); i < items.length; i++) {
            items[i].style.display = 'none';
            items[i].style.opacity = '0';
        }
        document.querySelector('.editor .textarea.' + newPreference).style.display = 'block';
        document.querySelector('.editor .textarea.' + newPreference).style.opacity = '0';

        setTimeout(function() {
            document.querySelector('.editor .textarea.' + newPreference).style.opacity = '1';
            if(aceEditor) {
                aceEditor.resize();
            }
        }, 500);

        document.querySelector('.highlighting select').value = newPreference;
    }

    document.querySelector('.platform select').onchange = function(e) {
        var tinyText = '';
        switch(document.querySelector('.platform select').value.split(' ').pop()) {
            case 'gb': tinyText = 'GB'; break;
            case 'nes': tinyText = 'NES'; break;
            case '2600': tinyText = '2600'; break;
            case 'c64': tinyText = 'C64'; break;
        }

        document.querySelector('.platform .tiny').innerHTML = tinyText;
    }

    document.querySelector('.highlighting select').onchange = function(e) {
        switchEditor(document.querySelector('.highlighting select').value);
    };

    wiz.compile.clearLog = function() {
        document.querySelector('.editor').style.bottom = '';
        if(aceEditor) {
            aceEditor.resize();
        }
        document.querySelector('.output').style.display = 'block';
        document.querySelector('.output').style.opacity = '1';

        document.querySelector('.console').innerHTML = '';
    }

    wiz.compile.log = function(message) {
        console.log(message);
        document.querySelector('.console').innerHTML += message + '\n';
    }

    document.querySelector('.output .close').onclick = function() {
        document.querySelector('.editor').style.bottom = '0';
        if(aceEditor) {
            aceEditor.resize();
        }
        document.querySelector('.output').style.opacity = '0';
        setTimeout(function() {
            document.querySelector('.output').style.display = 'none';
        }, 300);
    }

    document.querySelector('.output').style.display = 'none';
    document.querySelector('.output .close').onclick();

    var openFileDialog = function() {
        var click = document.createEvent("MouseEvent");
        click.initMouseEvent('click', true, true, window, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
        var input = document.createElement('input');
        input.type = 'file';
        input.dispatchEvent(click);
    };

    function addFile(filename, contents) {
        var tabs = document.querySelector('.tabs');
        var active = tabs.querySelector('.tab.active')
        if(active) {
            active.className = 'tab';
        }

        var tab = document.createElement('span');
        tab.className = 'tab active';
        tab.appendChild(document.createTextNode(filename));

        var close = document.createElement('span');
        close.className = 'close';
        close.appendChild(document.createTextNode('\xD7'));
        tab.appendChild(close);
        tabs.insertBefore(tab, tabs.querySelector('.add'));

        updateTabs();
    }

    function updateTabs() {
        var tabs = document.querySelector('.tabs');
        for(var i = 0, match = tabs.querySelectorAll('.tab'), len = match.length; i < len; i++) {
            (function() {
                var tab = match[i];
                tab.onclick = function() {
                    var active = tabs.querySelector('.tab.active')
                    if(active) {
                        active.className = 'tab';
                    }
                    tab.className = 'tab active';
                    return false;
                };

                var button = tab.querySelector('.close');
                if(button.className === 'close') {
                    button.onclick = function() {
                        if(window.confirm('Are you sure you want to remove "' + tab.childNodes[0].nodeValue + '"?')) {
                            tabs.removeChild(tab);

                            if(!tabs.querySelector('.tab.active')) {
                                var next = tabs.querySelector('.tab');
                                if(next) {
                                    next.className = 'tab active';
                                }
                            }
                            updateTabs();
                        }
                        return false;
                    };
                } else if(button.className === 'close startup') {
                    button.onclick = function() {
                        var filename = window.prompt('Enter a new filename for this project.', tab.childNodes[0].nodeValue);
                        if(filename) {
                            tab.replaceChild(document.createTextNode(filename), tab.childNodes[0]);
                        }
                        updateTabs();
                    };
                }
            })();
        }
    }
    updateTabs();

    document.querySelector('.tabs .add').onclick = function() {
        var filename = window.prompt('Enter a filename.', '');
        if(filename) {
            addFile(filename, '');
        }
    }

    document.querySelector('.tabs .load').onclick = function() {
        openFileDialog();
    }

    var saveFileDialog =
        window.webkitSaveAs
        || window.mozSaveAs
        || window.msSaveAs
        || window.navigator.msSaveBlob && function(blob, name) {
                    return window.navigator.msSaveBlob(blob, name);
            }
        || function(blob, name) {
            var click = document.createEvent("MouseEvent");
            click.initMouseEvent('click', true, true, window, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
            var a = document.createElement('a');
            a.setAttribute('href', URL.createObjectURL(blob));
            a.setAttribute('download', name);
            a.dispatchEvent(click);
        };

    function dataUrlToBlob(url) {
        var parts = url.split(',');
        var contentType = parts[0].split(':')[1].split(';');
        var mimeType = contentType[0];
        var encoding = contentType[1];
        var data = parts[1];

        if (encoding == 'base64') {
            var bytes = window.atob(data);
            var buffer = new Uint8Array(bytes.length);
            for (var i = 0; i < bytes.length; ++i) {
                buffer[i] = bytes.charCodeAt(i);
            }
            return new Blob([buffer], {type: mimeType});
        } else {
            return new Blob([data], {type: mimeType});
        }
    }

    document.querySelector('.build').onclick = function() {
        wiz.build(getSource(), 'code.wiz', document.querySelector('.platform select').value.split(' ').shift());
    }

    document.querySelector('.play').onclick = function() {
        var rom = wiz.build(getSource(), 'code.wiz', document.querySelector('.platform select').value.split(' ').shift());
        wiz.compile.log('');
        wiz.compile.log('Emulation is not implemented yet!');
    }

    document.querySelector('.download').onclick = function() {
        var rom = wiz.build(getSource(), 'code.wiz', document.querySelector('.platform select').value.split(' ').shift());
        var blob = new Blob([rom], {type: "application/octet-stream"})
        saveFileDialog(blob, 'game.gb');
    }

    



    initAceEditor();

    if(document.cookie) {
        var cookie = document.cookie.split('; ');
        for(var i = 0; i < cookie.length; i++) {
            var pieces = cookie[i].split('=');
            var name = pieces[0];
            var value = pieces[1].split(',').shift();
            console.log(name, value);
            if(name === 'wiz_editor_preference') {
                switchEditor(value);
                return;
            }
        }
    }

    function touch(e) {
        if(!editorPreference) {
            switchEditor('text');
        }
        document.removeEventListener('touchstart', touch, true);
        document.removeEventListener('mousemove', mouse, true);
    }

    function mouse(e) {
        if(!editorPreference) {
            switchEditor('ace');
        }
        document.removeEventListener('touchstart', touch, true);
        document.removeEventListener('mousemove', mouse, true);
    }
    document.addEventListener('touchstart', touch, true);
    document.addEventListener('mousemove', mouse, true);
}

})(this);