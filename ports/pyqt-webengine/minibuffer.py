from PyQt5.QtWebEngineWidgets import QWebEngineView

import core_interface


class Minibuffer():
    """Documentation for Minibuffer

    """
    view = None
    scripts = {}
    callback_count = 0
    window_identifier = 0

    def __init__(self, window_identifier):
        super(Minibuffer, self).__init__()
        self.view = QWebEngineView()
        self.window_identifier = window_identifier
        self.view.setHtml("")  # breaks without this line

    def evaluate_javascript(self, script):
        """
        This method returns an identifier (str) to the LISP core. Upon
        completion of the javascript script, the platform port will
        make a call to the LISP core with the results of that
        computation, and the associated identifier.

        :param script: the JavaScript script to execute
        :returns: a callback_id
        :rtype: string
        """
        self.callback_count += 1
        self.view.page().runJavaScript(
            script,
            lambda x: self.javascript_callback(x, str(self.callback_count)))

    def javascript_callback(self, res, callback_id):
        if res is None:
            return
        core_interface.minibuffer_javascript_call_back(str(self.window_identifier), res, callback_id)

    def set_height(self, height):
        self.view.setFixedHeight(height)
