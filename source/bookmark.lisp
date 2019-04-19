;;; bookmark.lisp --- manage and create bookmarks

(in-package :next)

(defun initialize-bookmark-db ()
  "Create a database file if necessary and make a table for bookmarks"
  (unless (probe-file *bookmark-db-path*)
    (close (open *bookmark-db-path* :direction :probe :if-does-not-exist :create))
    (let ((db (sqlite:connect
               (truename (probe-file *bookmark-db-path*)))))
      (sqlite:execute-non-query
       db "create table bookmarks (id integer primary key, url text not null)")
      (sqlite:execute-non-query
       db "insert into bookmarks (url) values (?)" "about:blank")
      (sqlite:disconnect db))))

(defun bookmark-complete (input)
  (let* ((db (sqlite:connect
              (truename (probe-file *bookmark-db-path*))))
         (candidates
           (sqlite:execute-to-list
            db "select url from bookmarks where url like ?"
            (format nil "%~a%" input))))
    (sqlite:disconnect db)
    (reduce #'append candidates :from-end t)))

(defun %bookmark-url (url)
  (let ((db (sqlite:connect
             (truename (probe-file *bookmark-db-path*)))))
    (sqlite:execute-non-query
     db "insert into bookmarks (url) values (?)" url)
    (sqlite:disconnect db)))

(define-command bookmark-current-page ()
  "Bookmark the currently opened page in the active buffer."
  (with-result (url (buffer-get-url))
    (let ((db (sqlite:connect
               (truename (probe-file *bookmark-db-path*)))))
      (sqlite:execute-non-query
       db "insert into bookmarks (url) values (?)" url)
      (sqlite:disconnect db)))
  (echo *minibuffer* "Current page bookmarked."))

(define-command bookmark-url ()
  "Allow the user to bookmark a URL via minibuffer input."
  (with-result (url (read-from-minibuffer *minibuffer*
                                          :input-prompt "Bookmark URL:"))
    (%bookmark-url url)))

(define-command bookmark-delete ()
  "Delete a bookmark from the bookmark database."
  (with-result (bookmark (read-from-minibuffer
                          *minibuffer*
                          :input-prompt "Delete bookmark:"
                          :completion-function 'bookmark-complete))
    (let ((db (sqlite:connect
               (truename (probe-file *bookmark-db-path*)))))
      (sqlite:execute-non-query
       db "delete from bookmarks where url = ?" bookmark)
      (sqlite:disconnect db))))

(define-command bookmark-anchor ()
  "Show link hints on screen, and allow the user to bookmark one"
  (with-result* ((links-json (add-link-hints))
                 (selected-anchor (read-from-minibuffer
                                   *minibuffer*
                                   :input-prompt "Bookmark anchor:"
                                   :cleanup-function #'remove-link-hints)))
    (let* ((link-hints (cl-json:decode-json-from-string links-json))
           (selected-link (cadr (assoc selected-anchor link-hints :test #'equalp))))
      (when selected-link
        (%bookmark-url selected-link)))))

(define-command set-url-from-bookmark ()
  "Set the URL for the current buffer from a bookmark."
  (with-result (url (read-from-minibuffer
                     *minibuffer*
                     :input-prompt "Open bookmark:"
                     :completion-function 'bookmark-complete))
    (buffer-evaluate-javascript *interface*
                                (buffer-active *interface*)
                                (buffer-set-url url))))
