;;; global.lisp --- global variable and parameter declarations

(in-package :next)

(defvar *options* ()
  "The list of command line options.")
(defvar *free-args* ()
  "The list of positional command line arguments.")

(defvar *core-port* 8081
  "The XML-RPC server port of the Lisp core.")
(defvar *platform-port-socket* '(:host "localhost" :port 8082)
  "The XML-RPC remote socket of the platform-port.")

(defvar *minibuffer* nil
  "A variable to store the minibuffer.")
(defvar *available-hooks* (make-hash-table :test #'equalp)
  "A hash of all available hooks.")
(defvar *available-commands* (make-hash-table :test #'equalp)
  "A hash of all available commands.")
(defvar *deferred-variables* ()
  "A list of functions which set globals which are deferred until startup for evaluation.")
(defvar *deferred-mode-initializations* ()
  "A list of functions invoked on start for modes.")
(defvar *global-map* (make-hash-table :test 'equal)
  "A global key map, available in every mode/buffer.")
(defvar *character-conversion-table* (make-hash-table :test 'equalp)
  "A table used to convert between special characters across different operating systems.")
(defvar *scroll-distance* 50
  "The distance scroll-down or scroll-up will scroll.")
(defvar *horizontal-scroll-distance* 50
  "Horizontal scroll distance. The distance scroll-left or scroll-right
  will scroll.")
(defvar *current-zoom-ratio* 1.0
  "The current zoom relative to the default zoom.")
(defvar *zoom-ratio-step* 0.2
  "The step size for zooming in and out.")
(defvar *zoom-ratio-min* 0.2
  "The minimum zoom ratio relative to the default.")
(defvar *zoom-ratio-max* 5.0
  "The maximum zoom ratio relative to the default.")
(defvar *zoom-ratio-default* 1.0
  "The default zoom ratio.")
(defvar *swank-port* 4006
  "The port that swank will open a new server on (default Emacs slime port
  is 4005, default set to 4006 in Next to avoid collisions).")
(defvar *start-page-url* "https://next.atlas.engineer/quickstart"
  "The URL of the first buffer opened by Next when started.")
(defvar *default-new-buffer-url* "https://next.atlas.engineer/start"
  "The URL set to a new blank buffer opened by Next.")
(defvar *default-new-buffer-mode* nil
  "The mode a buffer will open in by default")
(defvar *package-symbols* nil
  "The package symbols available, populated by helper function
  load-package-symbols.")
(defvar *package-globals* nil
  "The package global variables available, populated by helper
  function load package-globals")
(defvar *minibuffer-closed-height* 0
  "The height of the minibuffer when closed.")
(defvar *minibuffer-open-height* 200
  "The height of the minibuffer when open.")
(defvar *minibuffer-echo-height* 25
  "The height of the minibuffer when echoing.")
(defvar *platform-port-poll-interval* 0.015
  "The speed at which to poll the XML-RPC endpoint of a platform-port
  to see if it is ready to begin accepting XML-RPC commands.")
(defvar *temp-directory* "/tmp/"
  "The path where to create the \"next-$USER\" subfolder for temporary data storage.
This is were the platform port log is stored.")
(defvar *minibuffer-style*
  (cl-css:css
   '((* :font-family "monospace,monospace"
        :font-size "14px")
     (body :border-top "4px solid dimgray"
           :margin "0"
           :padding "0 6px")
     ("#container" :display "flex"
                   :flex-flow "column"
                   :height "100%")
     ("#input" :padding "6px 0"
               :border-bottom "solid 1px lightgray")
     ("#completions" :flex-grow "1"
                     :overflow-y "auto"
                     :overflow-x "auto")
     ("#cursor" :background-color "gray"
                :color "white")
     ("#prompt" :padding-right "4px"
                :color "dimgray")
     (ul :list-style "none"
         :padding "0"
         :margin "0")
     (li :padding "2px")
     (.selected :background-color "gray"
                :color "white")))
  "The CSS applied to a minibuffer when it is set-up.")
(deferredvar *init-file-path* (xdg-config-home "init.lisp")
  "The path where the system will look to load an init file from.")
(deferredvar *history-db-path* (xdg-data-home "history.db")
  "The path where the system will create/save the history database.")
(deferredvar *bookmark-db-path* (xdg-data-home "bookmark.db")
  "The path where the system will create/save the bookmark database.")
(deferredvar *cookie-path-dir* (xdg-data-home)
  "The path for cookies in the GTK Version of Next")

(defparameter +version+
  (let ((version (asdf/component:component-version (asdf:find-system :next)))
        (directory (asdf:system-source-directory :next)))
    (or (ignore-errors
         (uiop:with-current-directory (directory)
           (multiple-value-bind (current-commit)
               (uiop:run-program (list "git" "describe" "--always")
                                 :output '(:string :stripped t))
             (multiple-value-bind (tag-commit)
                 (uiop:run-program (list "git" "describe" version "--always")
                                   :output '(:string :stripped t))
               (concatenate 'string
                            version
                            (when (string/= tag-commit current-commit)
                              (format nil "-~a" current-commit)))))))
        version)))
