;;; file-manager-mode.lisp -- Manage files.
;;;
;;; "file manager" is a big excessive for now. Currently, we can:
;;; - browse and open files from the downloads directory.

(in-package :next)

;; (define-mode file-manager-mode ()
;;     "Display files."
;;     ())

(defun open-file-fn-default (filename)
  "Open this file with `xdg-open'."
  (handler-case (uiop:run-program (list "xdg-open" (namestring filename)))
    ;; We can probably signal something and display a notification.
    (error (c) (format *error-output* "Error opening ~a: ~a~&" filename c))))

;; TODO: Remove `open-file-fn` (it's just a one-liner) and instead store the
;; "open-file-function" into a download-mode slot, which is then called from
;; `download-open-file' with `(funcall (open-file-function download-mode)
;; filename).
(defun open-file-fn (filename)
  "Open this file. `filename' is the full path of the file, as a string.
By default, try to open it with the system's default external program, using `xdg-open'.
The user can override this function to decide what to do with the file."
  (open-file-fn-default filename))

(defun open-file-from-directory-completion-fn (directory)
  (let ((filenames (uiop:directory-files directory)))
    (lambda (input)
      (fuzzy-match input filenames))))

(define-command open-file (root-mode &optional (interface *interface*))
  "Open a file from the filesystem.
Get files from the `download-manager::*default-download-directory*' directory."
  (let ((directory download-manager::*default-download-directory*))
    (with-result (filename (read-from-minibuffer
                            (minibuffer interface)
                            :input-prompt (file-namestring directory)
                            :completion-function (apply #'open-file-from-directory-completion-fn (list directory))))
      (open-file-fn filename))))

(define-key  "C-x C-f" #'open-file)
