(function(osimis) {

    function MobileInterfacePolicy() {
        var _this = this;

        // Create observable
        this.onUpdate = new osimis.Listener();

        // Manage portrait / landscape orientations
        // 1. Create a portrait orientation media query. See 
        // http://caniuse.com/#search=matchMedia for browser compatibility.
        var mediaQuery = window.matchMedia("(orientation: portrait)");
        var orientation = mediaQuery.matches ? 'portrait' : 'landscape';
        // 2. Register current state
        this._onOrientationChanged(mediaQuery);
        // 3. Listen to changes
        mediaQuery.addListener(function(mediaQuery) {
            var orientation = mediaQuery.matches ? 'portrait' : 'landscape';
            _this._onOrientationChanged(orientation);
        });
        // 4. Unlisten to changes on destroy
        // @todo Unlisten to changes on destroy

        // Hide scrollbar on iphone.
        // See http://stackoverflow.com/questions/6011223/how-to-completely-hide-the-navigation-bar-in-iphone-html5
        // @warning this may have undesired behavior on projects embedding webviewer without iframe.
        window.scrollTo(0, 1);

        // Prevent from scrolling
        document.body.addEventListener('touchmove', function(event) {
          event.preventDefault();
        }, false); 

        // Set initial UI values
        // 1. Disable toolbar on mobile
        this.enableToolbar = false;
        // 2. Disable left/right side on portrait orientation
        this.enableLayoutTopLeft = (orientation === 'landscape');
        this.enableLayoutTopRight = (orientation === 'landscape');
        this.enableLayoutLeft = (orientation === 'landscape');
        this.enableLayoutRight = (orientation === 'landscape');
        // 3. Never display aside handles on mobile (buttons toggling the aside)
        this.enableLayoutLeftHandles = false;
        this.enableLayoutRightHandles = false;
        // 4. Show notice on portrait orientation with 
        this.enableNotice = (orientation === 'portrait');
        this.noticeText = 'Change your mobile orientation to display series list.';
    }

    MobileInterfacePolicy.prototype.onUpdate = _.noop;

    MobileInterfacePolicy.prototype._onOrientationChanged = function(orientation) {
        // Hide scrollbar on iphone.
        // See http://stackoverflow.com/questions/6011223/how-to-completely-hide-the-navigation-bar-in-iphone-html5
        // @warning this may have undesired behavior on projects embedding webviewer without iframe.
        window.scrollTo(0, 1);
        
        // On portrait orientation
        if (orientation === 'portrait') {
            // Disable left layout
            this.enableLayoutLeft = false;
            this.enableLayoutRight = false;
            
            // Disable top side
            this.enableLayoutTopLeft = false;
            this.enableLayoutTopRight = false;

            // Show text notice
            this.enableNotice = true;
        }

        // On landscape orientation
        if (orientation === 'landscape') {
            // Enable left/right layout
            this.enableLayoutLeft = true;
            this.enableLayoutRight = true;

            // Enable top side
            this.enableLayoutTopLeft = true;
            this.enableLayoutTopRight = true;

            // Hide text notice
            this.enableNotice = false;
        }

        // Trigger changes, so AngularJS directives can apply the interface
        // updates to their viewmodel and trigger a $digest cycle. This is only 
        // used by the `wvWebviewer` directive.
        this.onUpdate.trigger();
    }

    osimis.MobileInterfacePolicy = MobileInterfacePolicy;

})(this.osimis || (this.osimis = {}));